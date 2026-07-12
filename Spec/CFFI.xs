// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// C foreign-function interface:
//
// CFFI is opt-in. There is no automatic import and no automatic scope
// injection. The official module is the standard CFFI module. User code imports
// it explicitly:

imports CFFI;


// extern function declarations

#[repr(C)]
extern "C" {
    #[LinkName("puts")]
    #[NoUnwind]
    fn puts(text: CFFI.CStr) => Int;

    #[LinkName("strlen")]
    #[NoUnwind]
    fn strlen(text: CFFI.CStr) => ULong;
}


fn PrintLine(message: Str) => Result.Result<(), Result.Error> {
    c_text: CFFI.CString = CFFI.CString.fromStr(message)@;
    else: puts(c_text.asCStr());
    return Result.Ok(());
}


// library and header metadata

#[LinkLibrary("c")]
#[Header("stdio.h")]
#[repr(C)]
extern "C" {
    fn printf(format: CFFI.CStr, args: CFFI.VarArgs) => Int;
}

#[LinkLibrary("m")]
#[Header("math.h")]
#[repr(C)]
extern "C" {
    #[LinkName("cos")]
    fn c_cos(value: Double) => Double;
}


// explicit calling convention and symbol controls

#[Abi("C")]
#[CallingConvention(Cdecl)]
#[repr(C)]
extern "C" {
    #[ExportName("xs_plugin_init")]
    #[SymbolVisibility(Default)]
    fn plugin_init() => Int;
}


// C layout types

#[repr(C)]
data CPoint {
    x: Double,
    y: Double,
}

#[repr(C)]
#[packed(1)]
data PackedHeader {
    tag: UShort,
    length: UInt,
}

#[transparent]
data FileDescriptor {
    value: Int,
}


// pointers, ownership, and unsafe boundaries

#[Unsafe]
#[repr(C)]
extern "C" {
    fn malloc(size: ULong) => CFFI.RawPtr<Void>;

    fn free(ptr: CFFI.RawPtr<Void>);
}

fn Allocate(size: ULong) => Result.Result<CFFI.RawPtr<Void>, Result.Error> {
    ptr: CFFI.RawPtr<Void> = malloc(size);
    if (ptr.isNull()) {
        return Result.Error({ message: "allocation failed" });
    }
    return Result.Ok(ptr);
}


// callbacks

data CompareContext {
    order: Int,
}

#[CFunctionPointer]
type CompareFn = fn(
    left: CFFI.RawPtr<Void>,
    right: CFFI.RawPtr<Void>,
    context: CFFI.RawPtr<CompareContext>,
) => Int;

#[repr(C)]
extern "C" {
    #[LinkName("qsort_r")]
    fn qsort_r(
        base: CFFI.RawPtr<Void>,
        count: ULong,
        width: ULong,
        compare: CompareFn,
        context: CFFI.RawPtr<CompareContext>,
    );
}


// thread/async-aware FFI markers

#[repr(C)]
extern "C" {
    #[ForeignThreadSafe]
    #[NoCallbackIntoRuntime]
    fn thread_safe_poll(handle: CFFI.RawPtr<Void>) => Int;

    #[MayBlock]
    #[CancellationUnsafe]
    fn blocking_read(fd: Int, buffer: CFFI.RawPtr<u8>, length: ULong) => Long;
}


// Strong CFFI surface model:
//
// - CFFI.RawPtr<T>: nullable raw pointer.
// - CFFI.NonNull<T>: non-null raw pointer.
// - CFFI.CStr: borrowed null-terminated C string.
// - CFFI.CString: owned null-terminated C string.
// - CFFI.Slice<T>: pointer plus length view for explicit FFI boundaries.
// - CFFI.VarArgs: marker for C varargs positions.
// - CFFI.Handle<T>: opaque native handle with explicit ownership policy.
// - CFFI.Owned<T>, CFFI.Borrowed<T>, and CFFI.Out<T>: FFI ownership markers.
//
// CFFI does not infer safety. Calls marked Unsafe require an unsafe boundary in
// the completed language. Layout, ABI, unwind, thread, callback, ownership, and
// lifetime checks are verified by HIR/MIR before backend lowering.
