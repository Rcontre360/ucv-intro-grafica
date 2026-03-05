# Proyecto 2 

Este es un visor de modelos 3D que permite cargar, ver y editar archivos en formato OBJ.

## Cómo ejecutar el proyecto

1.  Abre una terminal en el directorio raíz del proyecto.
2.  Ejecuta el comando `make`. Esto compilará el proyecto y creará un ejecutable en `build/proy2`.
3.  Ejecuta el programa con `./build/proy2`.

## Características

### Menú de Archivo

*   **Load**: Abre un explorador de archivos para seleccionar un archivo `.obj`.
*   **Save**: Guarda el estado actual del objeto (incluyendo traslaciones, rotaciones, escalas y colores de sub-mallas) en un nuevo archivo `.obj` y su `.mtl` correspondiente.

### Controles del Ratón

*   **Click Izquierdo**: Selecciona una sub-malla del objeto. La sub-malla seleccionada se resaltará con su caja envolvente.
*   **Arrastrar con Click Izquierdo**:
    *   Si el modo "Move full object" está desactivado, traslada la sub-malla seleccionada.
    *   Si el modo "Move full object" está activado, traslada el objeto completo.
*   **Arrastrar con Click Derecho**: Rota el objeto sobre su centro.

### Controles del Teclado

*   **Flechas Arriba/Abajo**: Mueve la cámara hacia adelante y hacia atrás.
*   **Flechas Izquierda/Derecha**: Rota la cámara.
*   **Barra Espaciadora**: Activa/desactiva el modo "FPS", donde el movimiento del ratón controla la rotación de la cámara.
*   **W/S (en modo FPS)**: Mueve la cámara hacia adelante y hacia atrás.
*   **ESC**: Cierra la aplicación.

### Panel de Configuración

El panel de la izquierda contiene varias secciones para modificar el objeto y la visualización:

#### Basic

*   **Show fill**: Muestra/oculta el relleno de los triángulos del objeto.
*   **Move full object**: Alterna entre mover el objeto completo o una sub-malla individual con el ratón.
*   **Background**: Cambia el color de fondo de la escena.
*   **Scale X, Y, Z**: Escala el objeto en los ejes X, Y o Z.
*   **Center**: Centra el objeto en la escena y restaura su escala original.
*   **Show FPS**: Muestra/oculta el contador de fotogramas por segundo. El calculo lo realiza FPSCounter

#### Vertex

*   **Show**: Muestra/oculta los vértices del objeto.
*   **Size**: ajusta el tamaño de los vértices.
*   **Color**: Cambia el color de los vértices.

#### Wireframe

*   **Show**: Muestra/oculta el modo de alambre (wireframe) del objeto.
*   **Color**: Cambia el color del wireframe.

#### Normals

*   **Show**: Muestra/oculta las normales de los vértices.
*   **Color**: Cambia el color de las normales.
*   **Size**: ajusta la longitud de las normales. 1-100 siendo el porcentaje de la longitud de la diagonal.

#### Advanced

*   **Antialiasing**: Activa/desactiva el antialiasing para las líneas.
*   **Back-face Culling**: Activa/desactiva el descarte de caras traseras.
*   **Depth Test**: Activa/desactiva el test de profundidad.

#### Selected Submesh

Esta sección aparece cuando se selecciona una sub-malla.

*   **Delete Submesh**: Elimina la sub-malla seleccionada.
*   **Color**: Cambia el color de la sub-malla seleccionada.
*   **Bounding Box Color**: Cambia el color de la caja envolvente de la sub-malla seleccionada.
