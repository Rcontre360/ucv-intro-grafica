# Project #3 - 3D Rendering Engine (Miniature Scene)

## 🌟 Introduction
Welcome to **Project #3**, a 3D visualization experience designed to immerse the user in a "miniature" thematic world. The entire scene is set atop a wooden table, where common objects like a house, a pot, and Christmas decorations take on a colossal scale relative to the viewer. You take on the role of a "mini-man" exploring this desktop landscape, navigating through a snowy atmosphere illuminated by dancing light sources.

![Application Preview](assets/app.png)

A high-performance C++ 3D rendering engine built with OpenGL 3.3, featuring advanced lighting, texture mapping, and an interactive UI.

---

## 🚀 How to Run

### Linux
1. **Prerequisites**: Install GLFW and GLM (e.g., `sudo apt install libglfw3-dev libglm-dev`).
2. **Build**: Run `make` in the root directory.
3. **Execute**: Run `./build/proy3`.

### Windows
1. **Prerequisites**: Ensure you have a C++ compiler (like MinGW/GCC or MSVC) and the GLFW development files.
2. **Build**: Use the provided `Makefile` with `make` (via MinGW) or link the files in `src/` manually in an IDE.
3. **Execute**: Run the generated `.exe` in the `build/` folder.

---

## 🛠️ Feature Map (PDF Requirements)

### 1. The Miniature Scene (Mini-man)
* **Concept**: The entire scene is set on a wooden table. The camera starts in **FPS Mode** at a height of `-1.5` (`targetY`), simulating a "miniature human" walking on the table surface.
* **Reflective Sphere**: Found at the center of the scene. Note that while it appears as one object, it is a complex entity composed of multiple parts (Submeshes).

### 2. Illumination & Lights
* **Three RGB Lights**: Three spheres (Red, Green, Blue) orbit the table in "harmonious" circular trajectories.
* **UI Controls**:
  * **Location**: Found under the **"Lights"** collapsing header.
  * **RGB/Intensity**: Use the Color Edit and Intensity sliders to modify each light.
  * **Attenuation (f_att)**: The "Global Attenuation" checkbox toggles the distance-based light falloff (worth 0.5pts).
  * **Animation Speed**: Modify how fast the "fireflies" move using the "Anim Speed" slider.
  * **Shading Modes**: Change each light between **Phong**, **Blinn-Phong**, and **Flat** using the dropdown menu.

### 3. Movement & Camera
* **Modes**: Switch between **FPS** (ignores Y) and **GOD** (fly mode) in the **"Movement"** header.
* **Controls**: Use `W/A/S/D` or `Arrow Keys` to move. Press `ENTER` to lock the mouse for camera rotation, and `ESC` to release it.

### 4. Animated Scene
* **Santa**: Flying and ground-based circular paths.
* **Snowmen**: Rotating animations on both red and blue variants.
* **Pines**: Dynamic scaling animations on specific trees.

### 5. Texture Mapping & Materials
* **Automatic Loading**: Textures from OBJ/MTL files are loaded automatically.
* **Interactive Mapping**:
  * **Location**: Select an object in the **"Selection"** box to open the **"Edit"** panel.
  * **s-mapping**: Choose between **Standard** (UV), **Spherical**, and **Squared**. To best see this effect, select the **Parametric Sphere** (giant floating snowball) at the center of the scene.
  * **o-mapping**: Switch between **Position** and **Normal** based coordinate generation.
* **Bump Mapping**: Use the "Set Bump Map" button in the Edit panel. Best visualized on the central **Parametric Sphere** to see the surface "crags" without changing the actual geometry.
* **Skybox**: A snowy environment map is used for the room background and reflections.

---

## ✨ Extra Features (Beyond the PDF)

* **Asynchronous Loading**: The engine uses a **separate worker thread** to load the OBJ scene and textures. This prevents the OS from "hanging" or freezing the window during the heavy I/O phase. A loading screen is shown until the GPU upload is ready.
* **Ambient Occlusion (AO)**: We utilize the **Ambient Texture** slot to render pre-baked AO maps. You can see these subtle, realistic shadows in the corners of the **Pot**, the **Trees**, the **House**, and the **Snowmen**.
* **Specular Mapping**: Specific objects like the **Pot** and **Blue Snowmen** use specular maps to define which parts are shinier than others (e.g., the enamel of the pot vs. the metal rim).
* **Environment Reflections**: The **Bauble/Decoration Ball** on the Christmas tree has a high reflectivity factor, acting as an opaque mirror that reflects the Skybox in real-time.
* **Custom Textures**: The UI allows you to **add any image file** as a Diffuse or Bump map to **any object** in the scene.
* **Light Shininess**: Each light's "focus" (shininess) can be adjusted, affecting how sharp or broad the specular reflections appear on surfaces.
* **Submesh Abstraction**: To keep things simple for the user, applying a texture at the Object level automatically updates every submesh within that model simultaneously.
* **High Complexity**: A large-scale scene with dozens of animated objects and complex hierarchies, all running at high FPS.
