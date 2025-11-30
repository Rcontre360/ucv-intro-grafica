// this "serde" is a library for serialization and deserialization into JSON
use serde::{Deserialize, Serialize};
use std::ops::{Add, Mul, Sub};

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

// implements the addition for point
impl Add for Point {
    type Output = Point;

    fn add(self, other: Point) -> Point {
        Point(self.0 + other.0, self.1 + other.1)
    }
}

// implements substraction for points
impl Sub for Point {
    type Output = Point;

    fn sub(self, other: Point) -> Point {
        Point(self.0 - other.0, self.1 - other.1)
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
}
