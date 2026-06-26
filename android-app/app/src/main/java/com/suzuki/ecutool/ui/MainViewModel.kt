package com.suzuki.ecutool.ui

import android.app.Application
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.os.IBinder
import android.util.Log
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.viewModelScope
import com.suzuki.ecutool.data.DTC
import com.suzuki.ecutool.data.SDSData
import com.suzuki.ecutool.data.SDSEcuInfo
import com.suzuki.ecutool.service.ConnectionState
import com.suzuki.ecutool.service.TcpConnectionService
import com.suzuki.ecutool.util.ParsedResponse
import com.suzuki.ecutool.util.SDSProtocol
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch

class MainViewModel(application: Application) : AndroidViewModel(application) {

    private var service: TcpConnectionService? = null
    private var streamJob: Job? = null
    private var pollJob: Job? = null
    private var pollIntervalMs: Long = 1000
    private var autoConnectDone = false

    private val _connectionState = MutableLiveData<ConnectionState>(ConnectionState.Disconnected())
    val connectionState: LiveData<ConnectionState> = _connectionState

    private val _liveData = MutableLiveData(SDSData())
    val liveData: LiveData<SDSData> = _liveData

    private val _ecuInfo = MutableLiveData(SDSEcuInfo())
    val ecuInfo: LiveData<SDSEcuInfo> = _ecuInfo

    private val _dtcList = MutableLiveData<List<DTC>>(emptyList())
    val dtcList: LiveData<List<DTC>> = _dtcList

    private val _statusMessage = MutableLiveData("")
    val statusMessage: LiveData<String> = _statusMessage

    private val _isStreaming = MutableLiveData(false)
    val isStreaming: LiveData<Boolean> = _isStreaming

    private val connection = object : ServiceConnection {
        override fun onServiceConnected(name: ComponentName, binder: IBinder) {
            service = (binder as TcpConnectionService.LocalBinder).getService()
            observeService()
        }

        override fun onServiceDisconnected(name: ComponentName) {
            service = null
            _connectionState.value = ConnectionState.Disconnected("Service disconnected")
        }
    }

    init {
        try {
            val ctx = application
            val intent = Intent(ctx, TcpConnectionService::class.java)
            ctx.bindService(intent, connection, Context.BIND_AUTO_CREATE)
            val prefs = ctx.getSharedPreferences("app_settings", 0)
            pollIntervalMs = prefs.getInt("poll_interval", 1000).toLong()
            Log.d(TAG, "ViewModel initialized")
        } catch (e: Exception) {
            Log.e(TAG, "ViewModel init failed", e)
        }
    }

    private fun observeService() {
        viewModelScope.launch {
            service?.connectionState?.collect { state ->
                _connectionState.value = state
                if (state is ConnectionState.Connected) {
                    _statusMessage.value = "Connected to ${state.host}:${state.port}"
                    startPolling()
                } else if (state is ConnectionState.Disconnected) {
                    stopStream()
                    stopPolling()
                }
            }
        }

        viewModelScope.launch {
            service?.responseFlow?.collect { raw ->
                val parsed = SDSProtocol.parseResponse(raw)
                handleResponse(parsed)
            }
        }

        // Auto-connect after binding
        if (!autoConnectDone) {
            autoConnectDone = true
            val ctx = getApplication<Application>()
            val prefs = ctx.getSharedPreferences("app_settings", 0)
            if (prefs.getBoolean("auto_connect", false)) {
                connect()
            }
        }
    }

    private fun handleResponse(response: ParsedResponse) {
        when (response) {
            is ParsedResponse.Status -> {
                _liveData.value = SDSProtocol.sdsDataFromStatus(response)
                _statusMessage.value = "Status updated"
            }
            is ParsedResponse.DTCList -> {
                _dtcList.value = response.codes
                _statusMessage.value = "DTCs: ${response.codes.size} found"
            }
            is ParsedResponse.ECUInfo -> {
                _ecuInfo.value = SDSEcuInfo(
                    vin = response.vin,
                    flashSize = response.flashSize,
                    calOffset = response.calOffset,
                    calSize = response.calSize
                )
                _statusMessage.value = "ECU Info received"
            }
            is ParsedResponse.StreamData -> {
                _liveData.value = SDSProtocol.sdsDataFromStatus(response.status)
            }
            is ParsedResponse.Pong -> {
                _statusMessage.value = "Pong"
            }
            is ParsedResponse.Ok -> {
                _statusMessage.value = "OK: ${response.message}"
            }
            is ParsedResponse.Error -> {
                _statusMessage.value = "Error: ${response.message}"
            }
            is ParsedResponse.Stopped -> {
                _statusMessage.value = "Communication stopped"
                stopStream()
            }
            is ParsedResponse.Unknown -> {
                _statusMessage.value = "Unknown: ${response.raw}"
            }
            is ParsedResponse.Empty -> {}
        }
    }

    fun updatePollInterval(ms: Int) {
        pollIntervalMs = ms.toLong()
        if (_connectionState.value is ConnectionState.Connected) {
            restartPolling()
        }
    }

    private fun startPolling() {
        stopPolling()
        pollJob = viewModelScope.launch {
            try {
                while (true) {
                    service?.sendCommand(SDSProtocol.encodeStatusRequest().decodeToString())
                    delay(pollIntervalMs)
                }
            } catch (_: kotlinx.coroutines.CancellationException) {
                // Normal cancellation on stop/restart
            }
        }
    }

    private fun stopPolling() {
        pollJob?.cancel()
        pollJob = null
    }

    private fun restartPolling() {
        startPolling()
    }

    fun connect(host: String = TcpConnectionService.DEFAULT_HOST, port: Int = TcpConnectionService.DEFAULT_PORT) {
        service?.connect(host, port)
    }

    fun disconnect() {
        service?.disconnect()
        _connectionState.value = ConnectionState.Disconnected()
        _statusMessage.value = "Disconnected"
    }

    fun requestDTC() {
        service?.sendCommand(SDSProtocol.encodeDTCRequest().decodeToString())
    }

    fun requestECUInfo() {
        service?.sendCommand(SDSProtocol.encodeInfoRequest().decodeToString())
    }

    fun dealerModeOn() {
        service?.sendCommand(SDSProtocol.encodeDealerOn().decodeToString())
        _statusMessage.value = "Dealer mode ON"
    }

    fun dealerModeOff() {
        service?.sendCommand(SDSProtocol.encodeDealerOff().decodeToString())
        _statusMessage.value = "Dealer mode OFF"
    }

    fun ping() {
        service?.sendCommand(SDSProtocol.encodePing().decodeToString())
        _statusMessage.value = "Ping sent"
    }

    fun startStream() {
        if (_isStreaming.value == true) return
        _isStreaming.value = true
        service?.sendCommand(SDSProtocol.encodeStreamStart().decodeToString())
        streamJob = viewModelScope.launch {
            while (_isStreaming.value == true) {
                delay(5000)
                service?.sendCommand(SDSProtocol.encodeStatusRequest().decodeToString())
            }
        }
    }

    fun stopStream() {
        _isStreaming.value = false
        streamJob?.cancel()
        service?.sendCommand(SDSProtocol.encodeStreamStop().decodeToString())
    }

    fun sendCommand(cmd: String) {
        service?.sendCommand(cmd)
        _statusMessage.value = "Sent: $cmd"
    }

    fun sendStop() {
        service?.sendCommand(SDSProtocol.encodeStop().decodeToString())
        _statusMessage.value = "Stop sent"
        stopStream()
    }

    fun clearStatusMessage() {
        _statusMessage.value = ""
    }

    override fun onCleared() {
        stopStream()
        stopPolling()
        try { getApplication<Application>().unbindService(connection) } catch (_: Exception) {}
        super.onCleared()
    }

    companion object {
        private const val TAG = "MainViewModel"
    }
}
