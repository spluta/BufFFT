//use crossbeam::thread;
use libc::c_float;
use ness_stretch_lib::process_chunk;
use ness_stretch_lib::NessStruct;
//use rand::Rng;

pub struct NessStretchR {
    ness_struct: NessStruct,
}

impl NessStretchR {
    fn new(dur_mult: f32, max_win_size: i32, win_size_divisor: i32, num_slices: i32, extreme: i32, paul_stretch_win_size: i32) -> NessStretchR {
        let mut filter_on = 1;
        if num_slices == 1 {
            filter_on = 0;
        }
        NessStretchR {
            ness_struct: NessStruct::new(
                dur_mult as f64,
                max_win_size as usize,
                win_size_divisor as usize,
                1,
                num_slices as usize,
                filter_on,
                extreme as usize,
                paul_stretch_win_size as usize,
                0,
            ),
        }
    }

    fn calc_block(&mut self) {
        let temp = process_chunk(&mut self.ness_struct);
        for iter in 0..self.ness_struct.max_win_size {
            //self.ness_struct.max_win_size
            self.ness_struct.in_chunk[0][iter] = temp[0][iter];
        }
    }
}

//new creates an object then passes a pointer to it over to the C++ client for safe keeping
#[no_mangle]
pub extern "C" fn ness_stretch_new(
    dur_mult: f32,
    max_win_size: i32,
    win_size_divisor: i32,
    num_slices: i32,
    extreme: i32,
    paul_stretch_win_size: i32,
) -> *mut NessStretchR {
    Box::into_raw(Box::new(NessStretchR::new(
        dur_mult,
        max_win_size,
        win_size_divisor,
        num_slices,
        extreme,
        paul_stretch_win_size
    )))
}

//I'm not sure if the object is being freed correctly
//clearly the compiler is unhappy, but I think it puts the ptr in the box then destroys it
#[no_mangle]
pub extern "C" fn ness_stretch_free(ptr: *mut NessStretchR) {
    if ptr.is_null() {
        return;
    }
    unsafe {
        let _ = Box::from_raw(ptr);
    }
}

//in order to get a value from the object we get the object pointer back from C++, dereference the raw pointer,
// then run the next function of the object
#[no_mangle]
pub extern "C" fn ness_stretch_next(
    ptr: *mut NessStretchR,
    out_buf: *mut c_float,
    size_out: i16,
    off_set: i32,
) {
    let ness_stretch = unsafe {
        assert!(!ptr.is_null());
        &mut *ptr
    };

    let out_vec: &mut [f32] = unsafe { std::slice::from_raw_parts_mut(out_buf, size_out as usize) };

    //if the waitCount is greater than 0, copy the buffer from the in_chunk
    //when the waitCount hits 0, copy the buffer from the stored_chunk

    for iter in 0..size_out as usize {
        out_vec[iter] = ness_stretch.ness_struct.stored_chunk[0][iter + off_set as usize] as f32;
    }
}

//in order to get a value from the object we get the object pointer back from C++, dereference the raw pointer,
// then run the next function of the object
#[no_mangle]
pub extern "C" fn ness_stretch_move_in_to_stored(ptr: *mut NessStretchR) {
    let ness_stretch = unsafe {
        assert!(!ptr.is_null());
        &mut *ptr
    };

    for iter in 0..ness_stretch.ness_struct.max_win_size {
        //self.ness_struct.max_win_size
        ness_stretch.ness_struct.stored_chunk[0][iter] = ness_stretch.ness_struct.in_chunk[0][iter];
    }
}

//in order to get a value from the object we get the object pointer back from C++, dereference the raw pointer,
// then run the next function of the object
#[no_mangle]
pub extern "C" fn ness_stretch_set_in_chunk(ptr: *mut NessStretchR, value: c_float, index: i32) {
    let ness_stretch = unsafe {
        assert!(!ptr.is_null());
        &mut *ptr
    };

    ness_stretch.ness_struct.in_chunk[0][index as usize] = value as f64;
}

//in order to get a value from the object we get the object pointer back from C++, dereference the raw pointer,
// then run the next function of the object
#[no_mangle]
pub extern "C" fn ness_stretch_calc(ptr: *mut NessStretchR) {
    //print!("{} ", size_out);

    let ness_stretch = unsafe {
        assert!(!ptr.is_null());
        &mut *ptr
    };

    ness_stretch.calc_block();
}
