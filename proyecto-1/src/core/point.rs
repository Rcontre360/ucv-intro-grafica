use std::ops::{Add, Sub};

use serde::{Deserialize, Serialize};

#[derive(Copy, Clone, Debug, PartialEq, Serialize, Deserialize)]
pub struct Point(pub i32, pub i32);

impl From<(i32, i32)> for Point {
    fn from(coords: (i32, i32)) -> Self {
        Point(coords.0, coords.1)
    }
}

impl From<(f32, f32)> for Point {
    fn from(coords: (f32, f32)) -> Self {
        Point(coords.0.round() as i32, coords.1.round() as i32)
    }
}

impl Add for Point {
    type Output = Point;

    fn add(self, other: Point) -> Point {
        Point(self.0 + other.0, self.1 + other.1)
    }
}

impl Sub for Point {
    type Output = Point;

    fn sub(self, other: Point) -> Point {
        Point(self.0 - other.0, self.1 - other.1)
    }
}

impl Point {
    pub fn interpolate(&self, other: Point, t: f32) -> Point {
        let x = (1.0 - t) * self.0 as f32 + t * other.0 as f32;
        let y = (1.0 - t) * self.1 as f32 + t * other.1 as f32;

        (x.round(), y.round()).into()
    }

    pub fn distance(&self, other: Point) -> f32 {
        let dx = self.0 - other.0;
        let dy = self.1 - other.1;
        // euclidean distance
        ((dx * dx + dy * dy) as f32).sqrt()
    }
}
