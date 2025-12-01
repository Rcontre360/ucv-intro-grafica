use super::line::draw_line;
use crate::canvas::Canvas;
use crate::core::{Point, ShapeCore, ShapeImpl, UpdateOp, RGBA};

/// detail factor is used to configure how much detail the bezier curve step t will have.
const DETAIL_FACTOR: f32 = 0.03;

pub struct Bezier {
    core: ShapeCore,
    subdivide_t: f32,
}

impl ShapeImpl for Bezier {
    fn new(core: ShapeCore) -> Bezier {
        Bezier {
            core,
            subdivide_t: 0.5,
        }
    }

    fn get_core(&self) -> ShapeCore {
        self.core.clone()
    }

    fn get_core_mut(&mut self) -> &mut ShapeCore {
        &mut self.core
    }

    fn update(&mut self, op: &UpdateOp) {
        self.update_basic(op);

        // bezier is the only one that also implements degree_elevate and subdivide update
        match op {
            UpdateOp::DegreeElevate => {
                self.degree_elevate();
            }
            UpdateOp::UpdateSubdivide(t) => {
                self.subdivide_t = *t;
            }
            _ => {}
        }
    }

    fn draw<'a>(&self, canvas: &mut Canvas<'a>) {
        draw_bezier(&self.core, canvas);
    }

    fn draw_with_color<'a>(&self, color: RGBA, canvas: &mut Canvas<'a>) {
        draw_bezier(&self.core.copy_with_color(color), canvas);
    }

    /// bezier on draw_selection also draws the subdivision point.
    /// we we modify this function to also draw those and the control points
    fn draw_selection<'a>(&self, color: RGBA, canvas: &mut Canvas<'a>) {
        self.draw_selection_basic(color, canvas);

        for i in 1..self.core.points.len() {
            let line_core = self
                .core
                .copy_with_points(vec![self.core.points[i - 1], self.core.points[i]]);
            draw_line(&line_core, canvas, true);
        }

        let p = de_casteljau(&self.core, self.subdivide_t);
        self.draw_control_point(p, color, canvas);
    }

    /// on the bezier hit test we pick the biggest possible square
    /// given the control points. Then we check if the point selected is within that square.
    /// This was discussed as the selection method for bezier in class but there are more precise ways to
    /// do it such as convex hull algorithms to get the convex polygon and then checking if the
    /// point falls within that polygon. That method is slower but more precise
    fn hit_test(&self, point: Point) -> bool {
        if self.core.points.is_empty() {
            return false;
        }

        let mut min_x = self.core.points[0].0;
        let mut max_x = self.core.points[0].0;
        let mut min_y = self.core.points[0].1;
        let mut max_y = self.core.points[0].1;

        for p in &self.core.points {
            min_x = min_x.min(p.0);
            max_x = max_x.max(p.0);
            min_y = min_y.min(p.1);
            max_y = max_y.max(p.1);
        }

        point.0 >= min_x && point.0 <= max_x && point.1 >= min_y && point.1 <= max_y
    }

    /// for subdivision we dont modify the current shape, we do that in the application state
    /// management. The subdivision only returns the resulting shapes if the method is implemented.
    /// For bezier subdivision creates the vectors for the two other shapes and runs an
    /// interpolation over the current points, then we fill both vectors with the first and last
    /// result from the recurrent interpolation.
    fn subdivide(&self) -> Option<(ShapeCore, ShapeCore)> {
        let mut pts_cpy = self.core.points.clone();
        let n = pts_cpy.len();

        let mut first = vec![self.core.points[0]];
        let mut second = vec![self.core.points[n - 1]];

        for r in 1..n {
            for i in 0..(n - r) {
                pts_cpy[i] = pts_cpy[i].interpolate(pts_cpy[i + 1], self.subdivide_t);
            }

            first.push(pts_cpy[0]);
            second.push(pts_cpy[n - r - 1]);
        }

        second.reverse();

        Some((
            self.core.copy_with_points(first),
            self.core.copy_with_points(second),
        ))
    }
}

/// only bezier implements degree elevate. We dont use this outside the bezier class so we dont
/// implement it on ShapeImpl
impl Bezier {
    /// degree elevate adds more control points to the current shape by performing a degree elevate
    fn degree_elevate(&mut self) {
        let n = self.core.points.len() as f32;
        // we create new points since the old ones are used for calculation
        let mut new_points: Vec<Point> = Vec::with_capacity(n as usize + 1);

        new_points.push(self.core.points[0]);

        for i in 1..(n as usize) {
            let b_prev = self.core.points[i - 1];
            let b = self.core.points[i];

            new_points.push(b.interpolate(b_prev, i as f32 / n));
        }

        new_points.push(*self.core.points.last().unwrap());

        // here thanks to references and borrowing in rust the original core.points are removed
        // from memory
        self.core.points = new_points;
    }
}

/// draws a bezier curve given a shape core. This separation allows us to draw it with different
/// colors if a bezier is selected
fn draw_bezier(core: &ShapeCore, canvas: &mut Canvas) {
    let mut t = 0.0;
    let detail = get_detail(core);
    let mut prev_pts: Option<Point> = None;
    let mut draw_last = false;

    while t <= 1.0 {
        let p = de_casteljau(core, t);
        if let Some(prev) = prev_pts {
            // since draw line must not draw the first point we sort the points backwards p first
            // then prev
            // that way we draw a line that is connected to the next one without overlapping
            // basically given a,b,c,d points from the bezier curve we draw lines [a,b),[b,c),[c,d]
            let line_core = core.copy_with_points(vec![p, prev]);
            draw_line(&line_core, canvas, draw_last);
        }

        prev_pts = Some(p);
        t += detail;
        // our lines are drawn from a to b like this [a,b). the last point must be drawn, when this
        // is true the full line is drawn [a,b]
        draw_last = t + detail > 1.0;
    }
}

/// bezier curve algorithm used. given a t and control points it calculates the next point in the
/// curve by interpolating the control points in a recursive way (without using actual recursion,
/// just loops).
fn de_casteljau(core: &ShapeCore, t: f32) -> Point {
    let mut pts_cpy = core.points.clone();

    for r in 1..pts_cpy.len() {
        for i in 0..(pts_cpy.len() - r) {
            pts_cpy[i] = pts_cpy[i].interpolate(pts_cpy[i + 1], t);
        }
    }

    pts_cpy[0]
}

/// get_detail gives us how much will t increase on each step
/// its calculated by getting the distance between the control points (adding them) and then
/// multiplying this by 1 / DETAIL_FACTOR
fn get_detail(core: &ShapeCore) -> f32 {
    // distance is initialized with 0.1 to avoid division by 0
    let mut distance = 0.01;
    let pts = &core.points;
    for i in 0..pts.len() - 1 {
        distance += pts[i].distance(pts[i + 1]);
    }

    let dist = distance * DETAIL_FACTOR;
    1.0 / dist
}
