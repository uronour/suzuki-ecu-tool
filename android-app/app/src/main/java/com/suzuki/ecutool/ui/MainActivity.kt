package com.suzuki.ecutool.ui

import android.os.Bundle
import android.widget.Button
import android.widget.EditText
import android.widget.ImageView
import android.widget.TextView
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import androidx.viewpager2.widget.ViewPager2
import com.google.android.material.tabs.TabLayout
import com.google.android.material.tabs.TabLayoutMediator
import com.suzuki.ecutool.R
import com.suzuki.ecutool.service.ConnectionState

class MainActivity : AppCompatActivity() {

    private val viewModel: MainViewModel by viewModels()
    private val configManager = DashConfigManager(this)

    private lateinit var statusIcon: ImageView
    private lateinit var statusText: TextView
    private lateinit var actionButton: Button
    private lateinit var viewPager: ViewPager2
    private lateinit var tabLayout: TabLayout
    private lateinit var terminalInput: EditText
    private lateinit var terminalSend: Button

    var dashboardFragment: DashboardFragment? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        statusIcon = findViewById(R.id.connectionIcon)
        statusText = findViewById(R.id.statusText)
        actionButton = findViewById(R.id.actionButton)
        viewPager = findViewById(R.id.viewPager)
        tabLayout = findViewById(R.id.tabLayout)
        terminalInput = findViewById(R.id.terminalInput)
        terminalSend = findViewById(R.id.terminalSend)

        setupViewPager()
        setupObservers()
        setupListeners()
    }

    private fun setupViewPager() {
        val dash = DashboardFragment()
        dashboardFragment = dash
        val config = configManager.load()
        dash.setConfig(config)
        dash.setTheme(resolveTheme(config.themeName))

        val fragments = listOf(
            dash, LiveDataFragment(), DiagnosticsFragment(),
            ECUToolsFragment(), SettingsFragment(), AboutFragment()
        )
        viewPager.adapter = PageAdapter(this, fragments)
        viewPager.offscreenPageLimit = 1

        TabLayoutMediator(tabLayout, viewPager) { tab, pos ->
            tab.text = when (pos) {
                0 -> getString(R.string.dashboard)
                1 -> getString(R.string.live_data)
                2 -> getString(R.string.diagnostics)
                3 -> getString(R.string.ecu_tools)
                4 -> getString(R.string.settings)
                5 -> getString(R.string.about)
                else -> ""
            }
        }.attach()
    }

    fun applyGaugeTheme(name: String) {
        val theme = resolveTheme(name)
        configManager.setTheme(name)
        dashboardFragment?.setTheme(theme)
    }

    fun applyDashConfig(config: DashConfig) {
        configManager.save(config)
        dashboardFragment?.setConfig(config)
        dashboardFragment?.setTheme(resolveTheme(config.themeName))
    }

    private fun resolveTheme(name: String): GaugeTheme = when (name) {
        "Classic" -> GaugeThemes.Classic
        "Night" -> GaugeThemes.Night
        else -> GaugeThemes.Sport
    }

    private fun setupObservers() {
        viewModel.connectionState.observe(this) { state ->
            when (state) {
                is ConnectionState.Connected -> {
                    statusIcon.setImageResource(R.drawable.ic_wifi_connected)
                    statusText.text = getString(R.string.connected)
                    actionButton.text = getString(R.string.disconnect)
                }
                is ConnectionState.Connecting -> {
                    statusIcon.setImageResource(R.drawable.ic_wifi_connecting)
                    statusText.text = getString(R.string.connecting)
                    actionButton.text = getString(R.string.disconnect)
                }
                is ConnectionState.Disconnected -> {
                    statusIcon.setImageResource(R.drawable.ic_wifi_disconnected)
                    statusText.text = state.reason?.let { "Disconnected: $it" } ?: getString(R.string.disconnected)
                    actionButton.text = getString(R.string.connect)
                }
                is ConnectionState.Error -> {
                    statusIcon.setImageResource(R.drawable.ic_wifi_disconnected)
                    statusText.text = "${getString(R.string.error)}: ${state.message}"
                    actionButton.text = getString(R.string.connect)
                }
            }
        }
    }

    private fun setupListeners() {
        actionButton.setOnClickListener {
            when (viewModel.connectionState.value) {
                is ConnectionState.Connected, is ConnectionState.Connecting ->
                    viewModel.disconnect()
                else -> viewModel.connect()
            }
        }
        terminalSend.setOnClickListener {
            val cmd = terminalInput.text.toString().trim()
            if (cmd.isNotEmpty()) { viewModel.sendCommand(cmd); terminalInput.text.clear() }
        }
    }
}
