use std::ops::{Add, Deref, Index};

#[derive(Copy, Clone, Debug)]
pub struct RGBA([u8; 4]);

impl RGBA {
    pub fn new(r: u8, g: u8, b: u8, a: u8) -> Self {
        [r, g, b, a].into()
    }

    pub fn is_transparent(&self) -> bool {
        self.0[3] == 0
    }
}

impl From<[u8; 4]> for RGBA {
    fn from(rgba: [u8; 4]) -> Self {
        RGBA(rgba)
    }
}

impl From<RGBA> for [u8; 4] {
    fn from(color: RGBA) -> Self {
        color.0
    }
}

impl Deref for RGBA {
    // dereferencing the RGBA struct to a [u8] slice.
    type Target = [u8];

    /// allows RGBA struct to be used directly as an immutable &[u8] slice,
    /// making it compatible with functions that expect raw byte data.
    /// now is possible to do &color and returning [u8] as result
    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl Index<usize> for RGBA {
    type Output = u8;

    fn index(&self, index: usize) -> &Self::Output {
        &self.0[index]
    }
}

impl Add for RGBA {
    type Output = RGBA;

    fn add(self, other: RGBA) -> RGBA {
        let alpha = self[3] as f32 / 255.0;
        let new_r = (self[0] as f32 * alpha + other[0] as f32 * (1.0 - alpha)) as u8;
        let new_g = (self[1] as f32 * alpha + other[1] as f32 * (1.0 - alpha)) as u8;
        let new_b = (self[2] as f32 * alpha + other[2] as f32 * (1.0 - alpha)) as u8;

        [new_r, new_g, new_b, 255].into()
    }
}
