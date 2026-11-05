package io.qivora.qivforge.ui.viewport

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.qivora.qivforge.engine.EngineStats
import io.qivora.qivforge.theme.LocalQivTheme

/**
 * Debug stats overlay badge per Doc 04 §6 and Doc 02 §16 AC1
 */
@Composable
fun StatsBadge(
    stats: EngineStats,
    modifier: Modifier = Modifier
) {
    val theme = LocalQivTheme.current

    val fpsColor = when {
        stats.fps >= 55.0f -> theme.success
        stats.fps >= 30.0f -> theme.warning
        else -> theme.error
    }

    Box(
        modifier = modifier
            .background(Color(0xCC121418), RoundedCornerShape(8.dp))
            .border(1.dp, theme.outline.copy(alpha = 0.5f), RoundedCornerShape(8.dp))
            .padding(horizontal = 10.dp, vertical = 6.dp)
    ) {
        Row(
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.spacedBy(10.dp)
        ) {
            Text(
                text = "%.1f FPS".format(stats.fps),
                color = fpsColor,
                fontSize = 12.sp
            )
            Text(
                text = "%.1f ms".format(stats.frameMs),
                color = Color.White,
                fontSize = 12.sp
            )
            Text(
                text = "${stats.drawCalls} DC",
                color = theme.accent,
                fontSize = 12.sp
            )
            Text(
                text = "${stats.triangles} Tri",
                color = theme.onSurfaceMuted,
                fontSize = 12.sp
            )
        }
    }
}
