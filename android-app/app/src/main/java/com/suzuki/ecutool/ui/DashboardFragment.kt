package com.suzuki.ecutool.ui

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.TextView
import androidx.fragment.app.Fragment
import androidx.fragment.app.activityViewModels
import com.suzuki.ecutool.R
import com.suzuki.ecutool.service.ConnectionState
import com.suzuki.ecutool.ui.DashConfig
import com.suzuki.ecutool.ui.GaugeTheme

class DashboardFragment : Fragment() {

    private val viewModel: MainViewModel by activityViewModels()
    private lateinit var btnConnect: Button
    private lateinit var statusLabel: TextView

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View {
        return inflater.inflate(R.layout.fragment_dashboard, container, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        btnConnect = view.findViewById(R.id.btnConnectNow)
        statusLabel = view.findViewById(R.id.statusLabel)

        btnConnect.setOnClickListener { viewModel.connect() }

        viewModel.connectionState.observe(viewLifecycleOwner) { state ->
            when (state) {
                is ConnectionState.Connected -> {
                    btnConnect.text = "Disconnect"
                    statusLabel.text = "Connected"
                }
                is ConnectionState.Connecting -> {
                    btnConnect.text = "Connecting..."
                    btnConnect.isEnabled = false
                    statusLabel.text = "Connecting..."
                }
                else -> {
                    btnConnect.text = "Connect"
                    btnConnect.isEnabled = true
                    statusLabel.text = "Not connected"
                }
            }
        }
    }

    fun setConfig(config: DashConfig) {}
    fun setTheme(theme: GaugeTheme) {}
}
