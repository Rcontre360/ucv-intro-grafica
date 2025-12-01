how should be the hit test for bezier?

if the user deselects a figure we are selecting outside, thus, creating a figure. 
Should we onnly create a figure when dragging the mouse?

how efficiency will be evaluated? for example our draw_line function uses a dot parameter (one more operation)

"Al utilizar SHIFT mientras se draggea con el ratón, la elipse se desplegará como
círculo, y el rectángulo como cuadrado. (0.5 puntos)". Que significa esto?


DADO
"Permitir la selección de un nivel de transparencia en cada color (borde, relleno,
color de polígono de control, vértices, etc). Al hacer setPixel, considerar el nivel de
transparencia del pixel para mezclarse el color que ya está dibujado en el búfer
(color_de_buffer[x][y] = color_de_buffer[x][y] * (1-alpha) + color * alpha).
Implementar este blending con cuidado, debido a las conversiones de tipo (1 punto)."
Porque dice que hay que tener cuidado con el blending?
