use egui::Options;
// this "serde" is a library for serialization and deserialization into JSON
use serde::{Deserialize, Serialize};
use std::{
    cmp::{max, min},
    ops::{Add, Mul, Sub},
};

//here we have a "#[derive]" keyword. Basically implements other functionality into this object
//for example, Clone allows this object to have the ".clone()" method, which makes a copy of it
//the Serialize and Deserialize allows it to be serialized by serde. The PartialEq allows it to
//have the == operator loaded (we can make comparisons with it)

/// This is a rust doc comment. It has "///" instead of "//".
/// Its used by third party tools to generate documentation

/// This Point struct holds x and y (a point).
#[derive(Copy, Clone, Debug, PartialEq, Serialize, Deserialize)]
pub struct Point(pub i32, pub i32);

//Cast to turn (i32,i32) into a point. Allos to call ".into()" to this type to cast it to Point
impl From<(i32, i32)> for Point {
    fn from(coords: (i32, i32)) -> Self {
        Point(coords.0, coords.1)
    }
}

// cast for (f32,f32). First it rounds the number to the closest int and then casts it to Point
impl From<(f32, f32)> for Point {
    fn from(coords: (f32, f32)) -> Self {
        Point(coords.0.round() as i32, coords.1.round() as i32)
    }
}

// cast for (f32,f32). First it rounds the number to the closest int and then casts it to Point
impl From<Point> for (f32, f32) {
    fn from(coords: Point) -> Self {
        (coords.0 as f32, coords.1 as f32)
    }
}

// implements the addition for point
impl Add for Point {
    type Output = Point;

    fn add(self, other: Point) -> Point {
        Point(self.0 + other.0, self.1 + other.1)
    }
}

// implements substraction for points against scalar
impl Add<i32> for Point {
    type Output = Point;

    fn add(self, other: i32) -> Self::Output {
        Point(self.0 + other, self.1 + other)
    }
}

// implements substraction for points
impl Sub<Point> for Point {
    type Output = Point;

    fn sub(self, other: Point) -> Self::Output {
        Point(self.0 - other.0, self.1 - other.1)
    }
}

// implements substraction for points against scalar
impl Sub<i32> for Point {
    type Output = Point;

    fn sub(self, other: i32) -> Self::Output {
        Point(self.0 - other, self.1 - other)
    }
}

impl Mul<Point> for i32 {
    type Output = Point;

    fn mul(self, p: Point) -> Self::Output {
        (self * p.0, self * p.1).into()
    }
}

impl Mul<Point> for f32 {
    type Output = Point;

    fn mul(self, p: Point) -> Self::Output {
        (self * p.0 as f32, self * p.1 as f32).into()
    }
}

// custom methods for point
impl Point {
    /// computes an interpolation between current point and the one passed as argument.
    /// The "t" argument specifies the weight of the interpolation, being 0 all the weight to the
    /// point calling this method
    pub fn interpolate(&self, other: Point, t: f32) -> Point {
        let x = (1.0 - t) * self.0 as f32 + t * other.0 as f32;
        let y = (1.0 - t) * self.1 as f32 + t * other.1 as f32;

        (x.round(), y.round()).into()
    }

    /// computes the euclidean distance between the current point and the one passed as argument
    pub fn distance(&self, other: Point) -> f32 {
        let dx = self.0 - other.0;
        let dy = self.1 - other.1;
        // euclidean distance
        ((dx * dx + dy * dy) as f32).sqrt()
    }

    /// computes the dot product between current point and another one
    pub fn dot(&self, other: Point) -> i32 {
        self.0 * other.0 + self.1 * other.1
    }

    /// checks if the current point (self) is within the box formed by p1 and p2
    pub fn is_within_box(&self, p1: Point, p2: Point, error: u32) -> bool {
        let x1 = p1.0;
        let y1 = p1.1;
        let x2 = p2.0;
        let y2 = p2.1;

        let mut min_x = min(x1, x2);
        let mut max_x = max(x1, x2);
        let mut min_y = min(y1, y2);
        let mut max_y = max(y1, y2);

        min_x -= error as i32;
        max_x += error as i32;
        min_y -= error as i32;
        max_y += error as i32;

        self.0 >= min_x && self.0 <= max_x && self.1 >= min_y && self.1 <= max_y
    }
}
