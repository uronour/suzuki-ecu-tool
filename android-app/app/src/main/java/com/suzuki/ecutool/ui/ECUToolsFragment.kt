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

class ECUToolsFragment : Fragment() {

    private val viewModel: MainViewModel by activityViewModels()
    private lateinit var infoText: TextView
    private lateinit var btnInfo: Button
    private lateinit var btnDealerOn: Button
    private lateinit var btnDealerOff: Button
    private lateinit var btnStream: Button
    private lateinit var btnStop: Button
    private lateinit var btnPing: Button
    private lateinit var statusText: TextView

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View {
        return inflater.inflate(R.layout.fragment_ecu_tools, container, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        infoText = view.findViewById(R.id.ecuInfoText)
        btnInfo = view.findViewById(R.id.btnECUInfo)
        btnDealerOn = view.findViewById(R.id.btnDealerOn)
        btnDealerOff = view.findViewById(R.id.btnDealerOff)
        btnStream = view.findViewById(R.id.btnStream)
        btnStop = view.findViewById(R.id.btnStop)
        btnPing = view.findViewById(R.id.btnPing)
        statusText = view.findViewById(R.id.ecuToolsStatus)

        btnInfo.setOnClickListener { viewModel.requestECUInfo() }
        btnDealerOn.setOnClickListener { viewModel.dealerModeOn() }
        btnDealerOff.setOnClickListener { viewModel.dealerModeOff() }
        btnPing.setOnClickListener { viewModel.ping() }
        btnStop.setOnClickListener { viewModel.sendStop() }

        viewModel.isStreaming.observe(viewLifecycleOwner) { streaming ->
            btnStream.text = if (streaming) getString(R.string.stop_stream) else getString(R.string.start_stream)
        }

        btnStream.setOnClickListener {
            if (viewModel.isStreaming.value == true) {
                viewModel.stopStream()
            } else {
                viewModel.startStream()
            }
        }

        viewModel.ecuInfo.observe(viewLifecycleOwner) { info ->
            val sb = StringBuilder()
            if (info.vin.isNotEmpty()) sb.append("VIN: ${info.vin}\n")
            if (info.flashSize > 0) sb.append("Flash: ${info.flashSize / 1024} KB\n")
            if (info.calSize > 0) sb.append("Cal Offset: 0x%X\nCal Size: %d bytes\n".format(info.calOffset, info.calSize))
            infoText.text = sb.toString().ifEmpty { "No ECU info yet" }
        }

        viewModel.statusMessage.observe(viewLifecycleOwner) { msg ->
            if (msg.isNotEmpty()) statusText.text = msg
        }
    }
}
