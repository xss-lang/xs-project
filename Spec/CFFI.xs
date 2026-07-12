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
    fn puts(text: STD.CFFI.CStr) => Int;

    #[LinkName("strlen")]
    #[NoUnwind]
    fn strlen(text: STD.CFFI.CStr) => ULong;
}


fn PrintLine(message: Str) => Result.Result<(), Result.Error> {
    c_text: STD.CFFI.CString = STD.CFFI.CString.fromStr(message)@;
    else: puts(c_text.asCStr());
    return Result.Ok(());
}


// library and header metadata

#[LinkLibrary("c")]
#[Header("stdio.h")]
#[repr(C)]
extern "C" {
    fn printf(format: STD.CFFI.CStr, args: STD.CFFI.VarArgs) => Int;

    #[ThreadLocal]
    #[LinkName("errno")]
    static errno: Int;

    #[LinkName("stdin")]
    static stdin_handle: STD.CFFI.RawPtr<STD.CFFI.FILE>;

    #[LinkName("stdout")]
    static stdout_handle: STD.CFFI.RawPtr<STD.CFFI.FILE>;
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

#[Opaque]
data NativeLibrary {
    handle: STD.CFFI.Handle<NativeLibrary>,
}


// dynamic library loading

#[DynamicLibrary]
#[NoUnwind]
fn OpenLibrary(path: Str) => Result.Result<NativeLibrary, Result.Error> {
    return STD.CFFI.DynamicLibrary.open(path);
}


// pointers, ownership, and unsafe boundaries

#[Unsafe]
#[repr(C)]
extern "C" {
    fn malloc(size: ULong) => STD.CFFI.RawPtr<Void>;

    fn free(ptr: STD.CFFI.RawPtr<Void>);
}

fn Allocate(size: ULong) => Result.Result<STD.CFFI.RawPtr<Void>, Result.Error> {
    ptr: STD.CFFI.RawPtr<Void> = malloc(size);
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
    left: STD.CFFI.RawPtr<Void>,
    right: STD.CFFI.RawPtr<Void>,
    context: STD.CFFI.RawPtr<CompareContext>,
) => Int;

#[repr(C)]
extern "C" {
    #[LinkName("qsort_r")]
    fn qsort_r(
        base: STD.CFFI.RawPtr<Void>,
        count: ULong,
        width: ULong,
        compare: CompareFn,
        context: STD.CFFI.RawPtr<CompareContext>,
    );
}


// owned/borrowed/out pointer contracts

#[repr(C)]
extern "C" {
    #[Ownership(ReturnsOwned)]
    fn make_buffer(length: ULong) => STD.CFFI.Owned<STD.CFFI.RawPtr<u8>>;

    #[Ownership(TakesOwnership)]
    fn destroy_buffer(buffer: STD.CFFI.Owned<STD.CFFI.RawPtr<u8>>);

    #[Ownership(Borrows)]
    fn inspect_buffer(buffer: STD.CFFI.Borrowed<STD.CFFI.RawPtr<u8>>, length: ULong) => Int;

    #[Ownership(OutParameter)]
    fn read_status(status: STD.CFFI.Out<Int>) => Int;
}


// thread/async-aware FFI markers

#[repr(C)]
extern "C" {
    #[ForeignThreadSafe]
    #[NoCallbackIntoRuntime]
    fn thread_safe_poll(handle: STD.CFFI.RawPtr<Void>) => Int;

    #[MayBlock]
    #[CancellationUnsafe]
    fn blocking_read(fd: Int, buffer: STD.CFFI.RawPtr<u8>, length: ULong) => Long;
}


// Strong CFFI surface model:
//
// - STD.CFFI.RawPtr<T>: nullable raw pointer.
// - STD.CFFI.NonNull<T>: non-null raw pointer.
// - STD.CFFI.CStr: borrowed null-terminated C string.
// - STD.CFFI.CString: owned null-terminated C string.
// - STD.CFFI.Slice<T>: pointer plus length view for explicit FFI boundaries.
// - STD.CFFI.VarArgs: marker for C varargs positions.
// - STD.CFFI.Handle<T>: opaque native handle with explicit ownership policy.
// - STD.CFFI.FILE: opaque C FILE handle marker.
// - STD.CFFI.Owned<T>, STD.CFFI.Borrowed<T>, and STD.CFFI.Out<T>: FFI ownership markers.
// - STD.CFFI.DynamicLibrary: runtime loader with explicit symbol lookup.
// - STD.CFFI.Symbol<T>: typed dynamic symbol wrapper.
//
// CFFI does not infer safety. Calls marked Unsafe require an unsafe boundary in
// the completed language. Layout, ABI, unwind, thread, callback, ownership, and
// lifetime checks are verified by HIR/MIR before backend lowering.
