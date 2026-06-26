package com.suzuki.ecutool.ui

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.AdapterView
import android.widget.Button
import android.widget.EditText
import android.widget.Spinner
import android.widget.TextView
import androidx.fragment.app.Fragment
import androidx.fragment.app.activityViewModels
import com.google.android.material.materialswitch.MaterialSwitch
import com.suzuki.ecutool.R
import com.suzuki.ecutool.service.ConnectionState
import com.suzuki.ecutool.service.TcpConnectionService

class SettingsFragment : Fragment() {

    private val viewModel: MainViewModel by activityViewModels()
    private lateinit var hostInput: EditText
    private lateinit var portInput: EditText
    private lateinit var statusText: TextView
    private lateinit var btnConnect: Button
    private lateinit var btnDisconnect: Button
    private lateinit var btnQuickConnect: Button
    private lateinit var swAutoConnect: MaterialSwitch
    private lateinit var spinnerPoll: Spinner
    private lateinit var swKeepScreenOn: MaterialSwitch
    private lateinit var swUnitKmh: MaterialSwitch

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View {
        return inflater.inflate(R.layout.fragment_settings, container, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        try {
            setupViews(view)
        } catch (e: Exception) {
            statusText?.text = "Error: ${e.message}"
            android.util.Log.e("SettingsFragment", "Setup failed", e)
        }
    }

    private fun setupViews(view: View) {
        if (!isAdded) return

        hostInput = view.findViewById(R.id.hostInput)
        portInput = view.findViewById(R.id.portInput)
        statusText = view.findViewById(R.id.settingsStatus)
        btnConnect = view.findViewById(R.id.btnConnect)
        btnDisconnect = view.findViewById(R.id.btnDisconnect)
        btnQuickConnect = view.findViewById(R.id.btnQuickConnect)
        swAutoConnect = view.findViewById(R.id.swAutoConnect)
        spinnerPoll = view.findViewById(R.id.spinnerPollInterval)
        swKeepScreenOn = view.findViewById(R.id.swKeepScreenOn)
        swUnitKmh = view.findViewById(R.id.swUnitKmh)

        hostInput.setText(TcpConnectionService.DEFAULT_HOST)
        portInput.setText(TcpConnectionService.DEFAULT_PORT.toString())

        val ctx = requireContext()
        val prefs = ctx.getSharedPreferences("app_settings", 0)

        swAutoConnect.isChecked = prefs.getBoolean("auto_connect", false)
        swKeepScreenOn.isChecked = prefs.getBoolean("keep_screen_on", true)

        val cfg = DashConfigManager(ctx)
        swUnitKmh.isChecked = cfg.load().unitKmh

        val pollValues = intArrayOf(500, 1000, 2000, 5000)
        val savedPoll = prefs.getInt("poll_interval", 1000)
        val pollIdx = pollValues.indexOfFirst { it == savedPoll }.coerceAtLeast(0)
        spinnerPoll.setSelection(pollIdx)

        btnConnect.setOnClickListener {
            val host = hostInput.text.toString().trim()
            val port = portInput.text.toString().trim().toIntOrNull() ?: TcpConnectionService.DEFAULT_PORT
            if (host.isNotEmpty()) { viewModel.disconnect(); viewModel.connect(host, port) }
        }

        btnDisconnect.setOnClickListener { viewModel.disconnect() }

        btnQuickConnect.setOnClickListener {
            viewModel.disconnect()
            viewModel.connect(TcpConnectionService.DEFAULT_HOST, TcpConnectionService.DEFAULT_PORT)
        }

        viewModel.connectionState.observe(viewLifecycleOwner) { state ->
            if (!isAdded) return@observe
            val connected = state is ConnectionState.Connected
            btnConnect.isEnabled = !connected
            btnDisconnect.isEnabled = connected
            btnQuickConnect.isEnabled = !connected
            statusText.text = when (state) {
                is ConnectionState.Connected -> "Connected to ${state.host}:${state.port}"
                is ConnectionState.Connecting -> "Connecting..."
                is ConnectionState.Disconnected -> state.reason?.let { "Disconnected: $it" } ?: "Disconnected"
                is ConnectionState.Error -> "Error: ${state.message}"
            }
        }

        swAutoConnect.setOnCheckedChangeListener { _, checked ->
            if (!isAdded) return@setOnCheckedChangeListener
            prefs.edit().putBoolean("auto_connect", checked).apply()
        }

        spinnerPoll.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>?, v: View?, pos: Int, id: Long) {
                if (!isAdded) return
                prefs.edit().putInt("poll_interval", pollValues[pos]).apply()
                viewModel.updatePollInterval(pollValues[pos])
            }
            override fun onNothingSelected(p: AdapterView<*>?) {}
        }

        swKeepScreenOn.setOnCheckedChangeListener { _, checked ->
            if (!isAdded) return@setOnCheckedChangeListener
            prefs.edit().putBoolean("keep_screen_on", checked).apply()
            if (isAdded) {
                requireActivity().window?.let { win ->
                    if (checked) win.addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
                    else win.clearFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
                }
            }
        }

        swUnitKmh.setOnCheckedChangeListener { _, checked ->
            if (!isAdded) return@setOnCheckedChangeListener
            val c = cfg.load().copy(unitKmh = checked)
            cfg.save(c)
            (requireActivity() as? MainActivity)?.applyDashConfig(c)
        }

        if (isAdded) {
            view.findViewById<Button>(R.id.themeSport)?.setOnClickListener { applyThemeSafe("Sport") }
            view.findViewById<Button>(R.id.themeClassic)?.setOnClickListener { applyThemeSafe("Classic") }
            view.findViewById<Button>(R.id.themeNight)?.setOnClickListener { applyThemeSafe("Night") }
        }

        setupToggle(view, R.id.swRpm, "rpm")
        setupToggle(view, R.id.swSpeedo, "speedo")
        setupToggle(view, R.id.swGear, "gear")
        setupToggle(view, R.id.swClock, "clock")
        setupToggle(view, R.id.swOdo, "odo")
        setupToggle(view, R.id.swFuel, "fuel")
        setupToggle(view, R.id.swPanel, "panel")
        setupToggle(view, R.id.swNeutral, "neutral")
        setupToggle(view, R.id.swFan, "fan")
        setupToggle(view, R.id.swClutch, "clutch")
        setupToggle(view, R.id.swSidestand, "sidestand")
        setupToggle(view, R.id.swCheckEng, "check_eng")
        setupToggle(view, R.id.swOil, "oil")
        setupToggle(view, R.id.swHiBeam, "hibeam")
        setupToggle(view, R.id.swTurns, "turns")
        setupToggle(view, R.id.swLargeKm, "km_large")

        if (isAdded && swKeepScreenOn.isChecked) {
            requireActivity().window?.addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        }
    }

    private fun applyThemeSafe(name: String) {
        if (!isAdded) return
        try {
            (requireActivity() as? MainActivity)?.applyGaugeTheme(name)
            statusText?.text = "$name theme"
        } catch (_: Exception) {}
    }

    private fun setupToggle(view: View, id: Int, key: String) {
        if (!isAdded) return
        val sw = view.findViewById<MaterialSwitch>(id) ?: return
        val ctx = requireContext()
        val cfg = DashConfigManager(ctx)
        val current = cfg.load()
        sw.isChecked = when (key) {
            "rpm" -> current.showRpmGauge; "speedo" -> current.showSpeedo
            "gear" -> current.showGear; "clock" -> current.showClock
            "odo" -> current.showOdometer; "fuel" -> current.showFuelGauge
            "panel" -> current.showCenterPanel; "neutral" -> current.showNeutral
            "fan" -> current.showFan; "clutch" -> current.showClutch
            "sidestand" -> current.showSidestand; "check_eng" -> current.showCheckEngine
            "oil" -> current.showOilLight; "hibeam" -> current.showHiBeam
            "turns" -> current.showTurnSignals; "km_large" -> current.showKmLarge
            else -> true
        }
        sw.setOnCheckedChangeListener { _, checked ->
            if (!isAdded) return@setOnCheckedChangeListener
            try {
                val c = cfg.load()
                val updated = when (key) {
                    "rpm" -> c.copy(showRpmGauge = checked); "speedo" -> c.copy(showSpeedo = checked)
                    "gear" -> c.copy(showGear = checked); "clock" -> c.copy(showClock = checked)
                    "odo" -> c.copy(showOdometer = checked); "fuel" -> c.copy(showFuelGauge = checked)
                    "panel" -> c.copy(showCenterPanel = checked); "neutral" -> c.copy(showNeutral = checked)
                    "fan" -> c.copy(showFan = checked); "clutch" -> c.copy(showClutch = checked)
                    "sidestand" -> c.copy(showSidestand = checked); "check_eng" -> c.copy(showCheckEngine = checked)
                    "oil" -> c.copy(showOilLight = checked); "hibeam" -> c.copy(showHiBeam = checked)
                    "turns" -> c.copy(showTurnSignals = checked); "km_large" -> c.copy(showKmLarge = checked)
                    else -> c
                }
                (requireActivity() as? MainActivity)?.applyDashConfig(updated)
            } catch (_: Exception) {}
        }
    }

}
