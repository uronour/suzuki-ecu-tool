package com.suzuki.ecutool.service

import android.app.Service
import android.content.Intent
import android.os.Binder
import android.os.IBinder
import android.util.Log
import kotlinx.coroutines.*
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import java.io.BufferedReader
import java.io.InputStreamReader
import java.io.OutputStreamWriter
import java.net.InetSocketAddress
import java.net.Socket

class TcpConnectionService : Service() {

    private val binder = LocalBinder()
    private val serviceScope = CoroutineScope(Dispatchers.IO + SupervisorJob())
    private var socket: Socket? = null
    private var writer: OutputStreamWriter? = null
    private var reader: BufferedReader? = null
    private var readerJob: Job? = null
    private var pingerJob: Job? = null

    private val _connectionState = MutableStateFlow<ConnectionState>(ConnectionState.Disconnected())
    val connectionState: StateFlow<ConnectionState> = _connectionState.asStateFlow()

    private val _responseFlow = MutableStateFlow("")
    val responseFlow: StateFlow<String> = _responseFlow.asStateFlow()

    private val _responses = mutableListOf<String>()
    val pendingResponses: List<String> get() = _responses.toList()

    inner class LocalBinder : Binder() {
        fun getService(): TcpConnectionService = this@TcpConnectionService
    }

    override fun onBind(intent: Intent?): IBinder = binder

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        return START_STICKY
    }

    fun connect(host: String, port: Int = 8899) {
        if (_connectionState.value is ConnectionState.Connected) return

        _connectionState.value = ConnectionState.Connecting
        serviceScope.launch {
            try {
                val sock = Socket()
                sock.connect(InetSocketAddress(host, port), 5000)
                sock.soTimeout = 30000
                socket = sock
                writer = OutputStreamWriter(sock.getOutputStream(), Charsets.US_ASCII)
                reader = BufferedReader(InputStreamReader(sock.getInputStream(), Charsets.US_ASCII))

                _connectionState.value = ConnectionState.Connected(host, port)
                startReader()
                startPinger()
            } catch (e: Exception) {
                Log.e(TAG, "Connection failed", e)
                _connectionState.value = ConnectionState.Error(e.message ?: "Connection failed")
                cleanup()
            }
        }
    }

    fun disconnect() {
        serviceScope.launch {
            sendCommand("stop")
            cleanup()
            _connectionState.value = ConnectionState.Disconnected()
        }
    }

    fun sendCommand(cmd: String) {
        writer?.let { w ->
            serviceScope.launch {
                try {
                    w.write(cmd)
                    w.write("\r\n")
                    w.flush()
                    Log.d(TAG, "Sent: $cmd")
                } catch (e: Exception) {
                    Log.e(TAG, "Send failed", e)
                    _connectionState.value = ConnectionState.Error("Send failed: ${e.message}")
                }
            }
        }
    }

    fun sendCommandAndWait(cmd: String, timeoutMs: Long = 3000): String? {
        val prevCount = _responses.size
        sendCommand(cmd)

        return runBlocking {
            withTimeoutOrNull(timeoutMs) {
                while (_responses.size <= prevCount) {
                    delay(50)
                }
                synchronized(_responses) {
                    if (_responses.size > prevCount) _responses.removeAt(prevCount) else null
                }
            }
        }
    }

    private fun startReader() {
        readerJob?.cancel()
        readerJob = serviceScope.launch {
            try {
                val lines = reader?.lines()?.iterator()
                while (isActive && lines?.hasNext() == true) {
                    val msg = lines.next()
                    Log.d(TAG, "Received: $msg")
                    _responseFlow.value = msg
                    synchronized(_responses) {
                        _responses.add(msg)
                    }
                }
            } catch (e: Exception) {
                if (e !is java.net.SocketTimeoutException && e !is java.io.IOException) {
                    Log.e(TAG, "Reader error", e)
                }
            } finally {
                if (_connectionState.value is ConnectionState.Connected) {
                    _connectionState.value = ConnectionState.Disconnected("Connection lost")
                    cleanup()
                }
            }
        }
    }

    private fun startPinger() {
        pingerJob?.cancel()
        pingerJob = serviceScope.launch {
            while (isActive) {
                delay(15000)
                if (_connectionState.value is ConnectionState.Connected) {
                    sendCommand("ping")
                }
            }
        }
    }

    private fun cleanup() {
        try {
            reader?.close()
            writer?.close()
            socket?.close()
        } catch (_: Exception) {}
        reader = null
        writer = null
        socket = null
        readerJob?.cancel()
        pingerJob?.cancel()
        synchronized(_responses) { _responses.clear() }
    }

    override fun onDestroy() {
        cleanup()
        serviceScope.cancel()
        super.onDestroy()
    }

    companion object {
        private const val TAG = "TcpConnectionService"
        const val ACTION_CONNECT = "com.suzuki.ecutool.CONNECT"
        const val ACTION_DISCONNECT = "com.suzuki.ecutool.DISCONNECT"
        const val EXTRA_HOST = "host"
        const val EXTRA_PORT = "port"
        const val DEFAULT_PORT = 8899
        const val DEFAULT_HOST = "192.168.1.200"
    }
}

sealed class ConnectionState {
    data class Connected(val host: String, val port: Int) : ConnectionState()
    object Connecting : ConnectionState()
    data class Disconnected(val reason: String? = null) : ConnectionState()
    data class Error(val message: String) : ConnectionState()
}
