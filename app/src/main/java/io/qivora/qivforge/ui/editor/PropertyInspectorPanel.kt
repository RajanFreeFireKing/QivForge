package io.qivora.qivforge.ui.editor

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.qivora.qivforge.theme.LocalQivTheme

@Composable
fun PropertyInspectorPanel(
    selectedActor: ActorItem?,
    posX: Float, posY: Float, posZ: Float,
    rotX: Float, rotY: Float, rotZ: Float,
    scaleX: Float, scaleY: Float, scaleZ: Float,
    onTransformChange: (px: Float, py: Float, pz: Float, rx: Float, ry: Float, rz: Float, sx: Float, sy: Float, sz: Float) -> Unit,
    modifier: Modifier = Modifier
) {
    val theme = LocalQivTheme.current

    Surface(
        color = theme.panel,
        modifier = modifier
            .fillMaxHeight()
            .width(240.dp)
            .border(1.dp, theme.outline)
    ) {
        if (selectedActor == null) {
            Box(modifier = Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                Text("Select an actor", color = theme.onSurfaceMuted, fontSize = 13.sp)
            }
        } else {
            LazyColumn(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(12.dp),
                verticalArrangement = Arrangement.spacedBy(14.dp)
            ) {
                // Header (Actor Name)
                item {
                    Text(
                        text = selectedActor.name,
                        color = theme.onSurface,
                        fontSize = 16.sp
                    )
                    Text("Type: ${selectedActor.type}", color = theme.accent, fontSize = 11.sp)
                    Divider(color = theme.outline, modifier = Modifier.padding(top = 8.dp))
                }

                // Transform Section (Doc 04 §6.5)
                item {
                    Text("Transform", color = theme.onSurface, fontSize = 14.sp)
                    Spacer(modifier = Modifier.height(8.dp))

                    Vector3Row(
                        label = "Position",
                        x = posX, y = posY, z = posZ,
                        theme = theme,
                        onChange = { nx, ny, nz -> onTransformChange(nx, ny, nz, rotX, rotY, rotZ, scaleX, scaleY, scaleZ) }
                    )

                    Spacer(modifier = Modifier.height(8.dp))
                    Vector3Row(
                        label = "Rotation",
                        x = rotX, y = rotY, z = rotZ,
                        theme = theme,
                        onChange = { nx, ny, nz -> onTransformChange(posX, posY, posZ, nx, ny, nz, scaleX, scaleY, scaleZ) }
                    )

                    Spacer(modifier = Modifier.height(8.dp))
                    Vector3Row(
                        label = "Scale",
                        x = scaleX, y = scaleY, z = scaleZ,
                        theme = theme,
                        onChange = { nx, ny, nz -> onTransformChange(posX, posY, posZ, rotX, rotY, rotZ, nx, ny, nz) }
                    )
                }

                // Component Properties
                item {
                    Divider(color = theme.outline)
                    Spacer(modifier = Modifier.height(8.dp))
                    Text("Components", color = theme.onSurface, fontSize = 14.sp)
                    Spacer(modifier = Modifier.height(6.dp))

                    Box(
                        modifier = Modifier
                            .fillMaxWidth()
                            .clip(RoundedCornerShape(8.dp))
                            .background(theme.panelAlt)
                            .padding(10.dp)
                    ) {
                        Text(
                            text = if (selectedActor.type == "LIGHT") "Light: Directional Sun (100k lux)"
                            else if (selectedActor.type == "CAMERA") "Camera: Perspective (FOV 60°)"
                            else "MeshRenderer: PBR Metallic-Roughness",
                            color = theme.onSurface,
                            fontSize = 12.sp
                        )
                    }
                }
            }
        }
    }
}

@Composable
private fun Vector3Row(
    label: String,
    x: Float, y: Float, z: Float,
    theme: io.qivora.qivforge.theme.ThemeTokens,
    onChange: (Float, Float, Float) -> Unit
) {
    Column(verticalArrangement = Arrangement.spacedBy(4.dp)) {
        Text(label, color = theme.onSurfaceMuted, fontSize = 11.sp)
        Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
            CoordField(label = "X", value = x, color = Color(0xFFEF4444), theme = theme, onVal = { onChange(it, y, z) })
            CoordField(label = "Y", value = y, color = Color(0xFF10B981), theme = theme, onVal = { onChange(x, it, z) })
            CoordField(label = "Z", value = z, color = Color(0xFF3B82F6), theme = theme, onVal = { onChange(x, y, it) })
        }
    }
}

@Composable
private fun RowScope.CoordField(
    label: String,
    value: Float,
    color: Color,
    theme: io.qivora.qivforge.theme.ThemeTokens,
    onVal: (Float) -> Unit
) {
    Row(
        modifier = Modifier
            .weight(1f)
            .height(30.dp)
            .clip(RoundedCornerShape(4.dp))
            .background(theme.panelAlt)
            .border(1.dp, theme.outline.copy(alpha = 0.5f), RoundedCornerShape(4.dp)),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Box(
            modifier = Modifier
                .width(18.dp)
                .fillMaxHeight()
                .background(color.copy(alpha = 0.2f)),
            contentAlignment = Alignment.Center
        ) {
            Text(label, color = color, fontSize = 10.sp)
        }
        Text(
            text = "%.1f".format(value),
            color = theme.onSurface,
            fontSize = 11.sp,
            modifier = Modifier.padding(start = 4.dp)
        )
    }
}
