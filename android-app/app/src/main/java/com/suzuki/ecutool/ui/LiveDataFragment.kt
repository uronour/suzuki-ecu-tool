package com.suzuki.ecutool.ui

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.fragment.app.Fragment
import androidx.fragment.app.activityViewModels
import com.suzuki.ecutool.R
import com.suzuki.ecutool.data.SDSData

class LiveDataFragment : Fragment() {

    private val viewModel: MainViewModel by activityViewModels()

    private lateinit var rpmValue: TextView
    private lateinit var speedValue: TextView
    private lateinit var coolantValue: TextView
    private lateinit var intakeTempValue: TextView
    private lateinit var tpsValue: TextView
    private lateinit var batteryValue: TextView
    private lateinit var gearValue: TextView
    private lateinit var mapValue: TextView
    private lateinit var o2Value: TextView
    private lateinit var stpsValue: TextView
    private lateinit var injValue: TextView
    private lateinit var ignValue: TextView
    private lateinit var iacValue: TextView
    private lateinit var baroValue: TextView
    private lateinit var neutralValue: TextView
    private lateinit var statusValue: TextView

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View {
        return inflater.inflate(R.layout.fragment_live_data, container, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        rpmValue = view.findViewById(R.id.rpmValue)
        speedValue = view.findViewById(R.id.speedValue)
        coolantValue = view.findViewById(R.id.coolantValue)
        intakeTempValue = view.findViewById(R.id.intakeTempValue)
        tpsValue = view.findViewById(R.id.tpsValue)
        batteryValue = view.findViewById(R.id.batteryValue)
        gearValue = view.findViewById(R.id.gearValue)
        mapValue = view.findViewById(R.id.mapValue)
        o2Value = view.findViewById(R.id.o2Value)
        stpsValue = view.findViewById(R.id.stpsValue)
        injValue = view.findViewById(R.id.injValue)
        ignValue = view.findViewById(R.id.ignValue)
        iacValue = view.findViewById(R.id.iacValue)
        baroValue = view.findViewById(R.id.baroValue)
        neutralValue = view.findViewById(R.id.neutralValue)
        statusValue = view.findViewById(R.id.statusValue)

        viewModel.liveData.observe(viewLifecycleOwner) { data -> updateUI(data) }
        viewModel.statusMessage.observe(viewLifecycleOwner) { msg ->
            if (msg.isNotEmpty()) statusValue.text = msg
        }
    }

    private fun updateUI(data: SDSData) {
        rpmValue.text = formatValue(data.rpm, "rpm")
        speedValue.text = formatFloat(data.speedKmh(), "km/h")
        coolantValue.text = formatValue(data.coolantTemp, "°C")
        intakeTempValue.text = formatValue(data.intakeAirTemp, "°C")
        tpsValue.text = formatValue(data.throttlePos, "%")
        batteryValue.text = formatFloat(data.batteryVoltage(), "V")
        gearValue.text = if (data.gearPos == 0) "N" else data.gearPos.toString()
        mapValue.text = formatValue(data.mapKpa, "kPa")
        o2Value.text = formatValue(data.o2Sensor, "mV")
        stpsValue.text = formatValue(data.stps, "%")
        injValue.text = formatFloat(data.injectorPulseMs(), "ms")
        ignValue.text = formatValue(data.ignitionTiming, "°")
        iacValue.text = formatValue(data.iacStep, "")
        baroValue.text = formatValue(data.baroKpa, "kPa")
        neutralValue.text = if (data.neutral) "Yes" else "No"
    }

    private fun formatValue(value: Int, unit: String): String {
        return if (unit.isEmpty()) "$value" else "$value $unit"
    }

    private fun formatFloat(value: Float, unit: String): String {
        return "%.1f $unit".format(value)
    }
}
