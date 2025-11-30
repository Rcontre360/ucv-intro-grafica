use serde::{Deserialize, Serialize};
use std::ops::{Add, Deref, Index};

/// object to hold an RGBA color
#[derive(Copy, Clone, PartialEq, Debug, Serialize, Deserialize)]
pub struct RGBA([u8; 4]);

// public custom functions
impl RGBA {
    /// creates a new RGBA color
    pub fn new(r: u8, g: u8, b: u8, a: u8) -> Self {
        [r, g, b, a].into()
    }

    /// checks if the current color is transparent
    /// used later to decide wether or not draw the inside of a shape
    pub fn is_transparent(&self) -> bool {
        self.0[3] == 0
    }
}

// implementation of the default function. Returns the color BLACK with 0 alpha
impl Default for RGBA {
    fn default() -> Self {
        RGBA([0, 0, 0, 0])
    }
}

// cast to move [u8;4] to RGBA
impl From<[u8; 4]> for RGBA {
    fn from(rgba: [u8; 4]) -> Self {
        RGBA(rgba)
    }
}

// cast to move RGBA to [u8;4]
impl From<RGBA> for [u8; 4] {
    fn from(color: RGBA) -> Self {
        color.0
    }
}

// allows RGBA struct to be used directly as an immutable &[u8] slice,
// making it compatible with functions that expect raw byte data.
// now is possible to do &color and returning [u8] as result
impl Deref for RGBA {
    // dereferencing the RGBA struct to a [u8] slice.
    type Target = [u8];

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

// allows us to INDEX the RGBA object and get the corresponding inner color
impl Index<usize> for RGBA {
    type Output = u8;

    fn index(&self, index: usize) -> &Self::Output {
        &self.0[index]
    }
}

// implementation of addition +
impl Add for RGBA {
    type Output = RGBA;

    // implements the addition taking in account ALPHA. The caller color alpha is used to
    // interpolate between the two colors. This way we can do color1 + color2 and get the
    // interpolation depending on the alpha of color1
    fn add(self, other: RGBA) -> RGBA {
        let alpha = self[3] as f32 / 255.0;
        let new_r = (self[0] as f32 * alpha + other[0] as f32 * (1.0 - alpha)) as u8;
        let new_g = (self[1] as f32 * alpha + other[1] as f32 * (1.0 - alpha)) as u8;
        let new_b = (self[2] as f32 * alpha + other[2] as f32 * (1.0 - alpha)) as u8;

        [new_r, new_g, new_b, 255].into()
    }
}
