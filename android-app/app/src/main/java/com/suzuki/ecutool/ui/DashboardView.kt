package com.suzuki.ecutool.ui

import android.content.Context
import android.graphics.*
import android.util.AttributeSet
import android.view.View
import com.suzuki.ecutool.data.SDSData
import java.text.SimpleDateFormat
import java.util.*

data class GaugeTheme(
    val name: String,
    val bgColor: Int,
    val cardBg: Int,
    val textPrimary: Int,
    val textSecondary: Int,
    val accent: Int,
    val accent2: Int,
    val rpmGreen: Int,
    val rpmYellow: Int,
    val rpmRed: Int,
    val needleColor: Int,
    val indicatorBg: Int,
    val warningRed: Int
)

object GaugeThemes {
    val Sport = GaugeTheme("Sport",
        Color.rgb(8, 8, 12), Color.rgb(18, 18, 28),
        Color.WHITE, Color.rgb(150, 150, 165),
        Color.rgb(220, 28, 28), Color.rgb(255, 60, 60),
        Color.rgb(0, 200, 80), Color.rgb(255, 180, 0), Color.RED,
        Color.RED, Color.rgb(25, 25, 38), Color.RED)

    val Classic = GaugeTheme("Classic",
        Color.rgb(18, 18, 14), Color.rgb(28, 28, 22),
        Color.rgb(235, 230, 215), Color.rgb(170, 165, 150),
        Color.rgb(200, 170, 100), Color.rgb(220, 200, 140),
        Color.rgb(100, 180, 80), Color.rgb(200, 160, 50), Color.rgb(200, 60, 40),
        Color.rgb(200, 170, 100), Color.rgb(32, 32, 26), Color.rgb(200, 60, 40))

    val Night = GaugeTheme("Night",
        Color.rgb(4, 7, 14), Color.rgb(10, 16, 28),
        Color.rgb(170, 215, 255), Color.rgb(90, 140, 195),
        Color.rgb(0, 175, 255), Color.rgb(55, 195, 255),
        Color.rgb(0, 200, 200), Color.rgb(0, 175, 255), Color.rgb(255, 45, 95),
        Color.rgb(0, 195, 255), Color.rgb(14, 20, 34), Color.rgb(255, 45, 95))
}

class TripComputer {
    var tripKm: Float = 0f
    var odometerKm: Float = 12500f
    var avgSpeed: Float = 0f
    var maxSpeed: Float = 0f
    var movingTime: Long = 0
    var fuelUsedMl: Float = 0f
    private var lastSpeed: Float = 0f
    private var lastTime: Long = 0

    fun update(speedKmh: Float, injectorMs: Float, nowMs: Long) {
        if (lastTime == 0L) { lastTime = nowMs; return }
        val dt = (nowMs - lastTime) / 1000f
        if (dt <= 0f || dt > 5f) { lastTime = nowMs; lastSpeed = speedKmh; return }

        val avgSpd = (speedKmh + lastSpeed) / 2f
        val distKm = avgSpd * dt / 3600f
        tripKm += distKm
        odometerKm += distKm
        if (speedKmh > maxSpeed) maxSpeed = speedKmh
        if (speedKmh > 0) movingTime += (dt * 1000).toLong()

        // rough fuel estimate: ~0.05ml/s per ms injector at idle, scales with load
        fuelUsedMl += injectorMs * dt * 0.003f

        lastSpeed = speedKmh
        lastTime = nowMs
    }

    fun resetTrip() { tripKm = 0f; maxSpeed = 0f; movingTime = 0; fuelUsedMl = 0f }
}

class DashboardView(context: Context, attrs: AttributeSet? = null) : View(context, attrs) {

    var data = SDSData()
        set(v) { field = v; postInvalidate() }

    var trip = TripComputer()
        set(v) { field = v; postInvalidate() }

    var config = DashConfig()
        set(v) { field = v; postInvalidate() }

    var theme: GaugeTheme = GaugeThemes.Sport
        set(v) { field = v; postInvalidate() }

    // UI-only indicators (not from ECU)
    var leftTurnBlinking = false
    var rightTurnBlinking = false
    var hiBeamOn = false
    var hasDtc = false

    private val pFill = Paint(Paint.ANTI_ALIAS_FLAG).apply { style = Paint.Style.FILL }
    private val pStroke = Paint(Paint.ANTI_ALIAS_FLAG).apply { style = Paint.Style.STROKE }
    private val pText = Paint(Paint.ANTI_ALIAS_FLAG)
    private val rect = RectF()
    private val timeFmt = SimpleDateFormat("HH:mm", Locale.getDefault())

    override fun onDraw(canvas: Canvas) {
        val w = width.toFloat(); val h = height.toFloat()
        val cx = w / 2f; val cy = h / 2f; val s = minOf(w, h)

        canvas.drawColor(theme.bgColor)

        if (config.showRpmGauge) drawRpmGauge(canvas, cx, cy, s)
        if (config.showSpeedo) drawSpeedo(canvas, cx, cy, s)
        if (config.showFuelGauge) drawFuelGauge(canvas, cx, h, s)
        if (config.showGear) drawGearDisplay(canvas, cx, cy, s)
        if (config.showCenterPanel) drawCenterPanel(canvas, cx, cy, s)
        if (config.showIndicators) drawIndicators(canvas, cx, h, s)
        if (config.showOdometer) drawOdometerPanel(canvas, cx, h, s)
        if (config.showKmLarge) drawSpeedDigital(canvas, cx, cy, s)
    }

    // ─── RPM GAUGE ───────────────────────────────────────────────
    private fun drawRpmGauge(canvas: Canvas, cx: Float, cy: Float, s: Float) {
        val r = s * 0.30f; val gap = s * 0.12f
        rect.set(cx - r - gap, cy - r - s * 0.02f, cx + r + gap, cy + r + s * 0.02f)
        val sweep = 220f; val start = 160f
        val rpm = data.rpm.coerceIn(0, 15000)
        val angle = (rpm / 15000f) * sweep

        pStroke.strokeWidth = s * 0.025f
        pStroke.color = theme.cardBg; canvas.drawArc(rect, start, sweep, false, pStroke)
        pStroke.color = theme.rpmGreen
        canvas.drawArc(rect, start, minOf(angle, sweep * 0.6f), false, pStroke)
        if (angle > sweep * 0.6f) {
            pStroke.color = theme.rpmYellow
            canvas.drawArc(rect, start + sweep * 0.6f, minOf(angle - sweep * 0.6f, sweep * 0.2f), false, pStroke)
        }
        if (angle > sweep * 0.8f) {
            pStroke.color = theme.rpmRed
            canvas.drawArc(rect, start + sweep * 0.8f, angle - sweep * 0.8f, false, pStroke)
        }
        drawArcTicks(canvas, cx, cy, r, s, start, sweep, 15, 0, 15000, true)
        drawNeedle(canvas, cx, cy, r * 0.85f, start + angle, s, theme.needleColor)
    }

    // ─── SPEEDOMETER ─────────────────────────────────────────────
    private fun drawSpeedo(canvas: Canvas, cx: Float, cy: Float, s: Float) {
        val r = s * 0.26f; val scx = cx + r + s * 0.12f
        rect.set(scx - r, cy - r - s * 0.02f, scx + r, cy + r + s * 0.02f)
        val sweep = 220f; val start = 160f
        val spd = data.speedKmh().coerceIn(0f, 300f)
        val angle = (spd / 300f) * sweep

        pStroke.strokeWidth = s * 0.022f
        pStroke.color = theme.cardBg; canvas.drawArc(rect, start, sweep, false, pStroke)
        pStroke.color = theme.accent2; canvas.drawArc(rect, start, angle, false, pStroke)
        drawArcTicks(canvas, scx, cy, r, s, start, sweep, 6, 0, 300, false)
        drawNeedle(canvas, scx, cy, r * 0.85f, start + angle, s, theme.accent2)
    }

    private fun drawArcTicks(canvas: Canvas, cx: Float, cy: Float, r: Float, s: Float, start: Float, sweep: Float, n: Int, vMin: Int, vMax: Int, labels: Boolean) {
        pStroke.strokeWidth = s * 0.006f; pText.textSize = s * 0.03f; pText.textAlign = Paint.Align.CENTER
        for (i in 0..n) {
            val a = Math.toRadians((start + (i.toFloat() / n) * sweep).toDouble())
            val ir = r * 0.82f; val or = r * 0.90f
            pStroke.color = if (i % 3 == 0 && labels) theme.textSecondary else theme.cardBg
            canvas.drawLine(cx + ir * cos(a), cy + ir * sin(a), cx + or * cos(a), cy + or * sin(a), pStroke)
            if (i % 3 == 0 && labels) {
                pText.color = theme.textSecondary
                val lr = r * 0.70f
                canvas.drawText("${vMin + (vMax - vMin) * i / n}", cx + lr * cos(a), cy + lr * sin(a) + s * 0.012f, pText)
            }
        }
    }

    private fun drawNeedle(canvas: Canvas, cx: Float, cy: Float, len: Float, angle: Float, s: Float, color: Int) {
        val rad = Math.toRadians(angle.toDouble())
        val nx = cx + len * cos(rad); val ny = cy + len * sin(rad)
        pStroke.strokeWidth = s * 0.017f; pStroke.color = color
        canvas.drawLine(cx, cy, nx, ny, pStroke)
        pFill.color = color; canvas.drawCircle(cx, cy, s * 0.02f, pFill)
    }

    private fun cos(a: Double) = Math.cos(a).toFloat()
    private fun sin(a: Double) = Math.sin(a).toFloat()

    // ─── DIGITAL SPEED ───────────────────────────────────────────
    private fun drawSpeedDigital(canvas: Canvas, cx: Float, cy: Float, s: Float) {
        val spd = data.speedKmh().coerceAtLeast(0f)
        pText.textSize = s * 0.12f; pText.textAlign = Paint.Align.CENTER; pText.color = theme.textPrimary
        canvas.drawText("%.0f".format(spd), cx, cy + s * 0.06f, pText)
        pText.textSize = s * 0.03f; pText.color = theme.textSecondary
        canvas.drawText(if (config.unitKmh) "km/h" else "mph", cx, cy + s * 0.12f, pText)
    }

    // ─── GEAR ────────────────────────────────────────────────────
    private fun drawGearDisplay(canvas: Canvas, cx: Float, cy: Float, s: Float) {
        val g = if (data.gearPos == 0) "N" else "${data.gearPos}"
        pText.textSize = s * 0.1f; pText.textAlign = Paint.Align.CENTER
        pText.color = if (data.gearPos == 0) theme.rpmGreen else theme.textPrimary
        canvas.drawText(g, cx - s * 0.22f, cy + s * 0.06f, pText)
    }

    // ─── FUEL GAUGE ──────────────────────────────────────────────
    private var fuelLevel = 0.7f // simulated until ECU provides it
    private fun drawFuelGauge(canvas: Canvas, cx: Float, h: Float, s: Float) {
        val x = cx + s * 0.40f; val y = h * 0.30f; val fw = s * 0.04f; val fh = s * 0.12f

        pStroke.strokeWidth = s * 0.006f; pStroke.color = theme.textSecondary
        canvas.drawRect(x - fw / 2, y - fh, x + fw / 2, y, pStroke)
        canvas.drawRect(x - fw * 0.4f, y - fh * 1.15f, x + fw * 0.4f, y - fh, pStroke)

        val fillH = fh * fuelLevel.coerceIn(0f, 1f)
        pFill.color = when {
            fuelLevel < 0.15f -> theme.warningRed
            fuelLevel < 0.30f -> Color.rgb(255, 180, 0)
            else -> theme.rpmGreen
        }
        canvas.drawRect(x - fw / 2 + s * 0.004f, y - fillH, x + fw / 2 - s * 0.004f, y - s * 0.004f, pFill)

        pText.textSize = s * 0.025f; pText.textAlign = Paint.Align.CENTER; pText.color = theme.textSecondary
        canvas.drawText("F", x, y + s * 0.025f, pText)
    }

    // ─── CENTER PANEL ────────────────────────────────────────────
    private fun drawCenterPanel(canvas: Canvas, cx: Float, cy: Float, s: Float) {
        val ty = cy + s * 0.18f; val ts = s * 0.03f; val gap = ts * 1.5f
        pText.textSize = ts; pText.textAlign = Paint.Align.CENTER

        data class Row(val label: String, val value: String)
        val rows = mutableListOf<Row>()
        if (config.showClock) rows.add(Row("", timeFmt.format(Date())))
        rows.add(Row("COOL", "${data.coolantTemp}°C"))
        rows.add(Row("BAT", "%.1fV".format(data.batteryVoltage())))
        rows.add(Row("IAT", "${data.intakeAirTemp}°C"))
        rows.add(Row("TPS", "${data.throttlePos}%"))
        rows.add(Row("MAP", "${data.mapKpa}kPa"))
        rows.add(Row("INJ", "%.1fms".format(data.injectorPulseMs())))

        rows.forEachIndexed { i, row ->
            val y = ty + i * gap
            if (row.label.isNotEmpty()) {
                pText.color = theme.textSecondary; canvas.drawText(row.label, cx, y, pText)
            }
            pText.color = if (row.label.isEmpty()) theme.accent2 else theme.textPrimary
            canvas.drawText(row.value, cx, y + gap * 0.45f, pText)
        }
    }

    // ─── ODOMETER / TRIP ─────────────────────────────────────────
    private fun drawOdometerPanel(canvas: Canvas, cx: Float, h: Float, s: Float) {
        val y = h * 0.15f
        pText.textAlign = Paint.Align.CENTER
        pText.textSize = s * 0.028f; pText.color = theme.textSecondary
        canvas.drawText("ODO", cx, y, pText)
        pText.textSize = s * 0.04f; pText.color = theme.textPrimary
        canvas.drawText("%.1f km".format(trip.odometerKm), cx, y + s * 0.05f, pText)

        pText.textSize = s * 0.024f; pText.color = theme.textSecondary
        canvas.drawText("TRIP", cx + s * 0.35f, y, pText)
        pText.textSize = s * 0.035f; pText.color = theme.accent2
        canvas.drawText("%.1f".format(trip.tripKm), cx + s * 0.35f, y + s * 0.04f, pText)
    }

    // ─── INDICATORS ──────────────────────────────────────────────
    private fun drawIndicators(canvas: Canvas, cx: Float, h: Float, s: Float) {
        val items = mutableListOf<Pair<String, Boolean>>()
        if (config.showNeutral) items.add("N" to data.neutral)
        if (config.showFan) items.add("FAN" to data.fanOn)
        if (config.showClutch) items.add("CLU" to data.clutchIn)
        if (config.showSidestand) items.add("STD" to data.sidestandDown)
        if (config.showCheckEngine) items.add("MIL" to hasDtc)
        if (config.showOilLight) items.add("OIL" to false)
        if (config.showHiBeam) items.add("HI" to hiBeamOn)
        if (config.showTurnSignals) { items.add("◄" to leftTurnBlinking); items.add("►" to rightTurnBlinking) }
        if (items.isEmpty()) return

        val inY = h - s * 0.07f; val iw = s * 0.07f
        val totalW = items.size * iw; val startX = cx - totalW / 2f
        pText.textSize = s * 0.03f; pText.textAlign = Paint.Align.CENTER

        items.forEachIndexed { i, (label, active) ->
            val x = startX + (i + 0.5f) * totalW / items.size
            val bg = if (active) when {
                label == "MIL" || label == "OIL" -> theme.warningRed
                label == "◄" || label == "►" -> if (active) theme.rpmGreen else theme.cardBg
                label == "HI" -> theme.accent2
                else -> theme.accent
            } else theme.cardBg

            pFill.color = bg
            canvas.drawRoundRect(x - iw * 0.4f, inY - iw * 0.35f, x + iw * 0.4f, inY + iw * 0.35f, s * 0.01f, s * 0.01f, pFill)
            pText.color = if (active) theme.bgColor else theme.textSecondary
            canvas.drawText(label, x, inY + s * 0.012f, pText)
        }
    }

    fun toggleFuelLevel() { fuelLevel = (fuelLevel + 0.1f).coerceAtMost(1f) }
    fun setFuelLevel(v: Float) { fuelLevel = v.coerceIn(0f, 1f) }
}
