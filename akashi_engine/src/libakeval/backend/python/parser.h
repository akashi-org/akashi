#pragma once

#include <libakcore/rational.h>
#include <libakcore/memory.h>
#include <libakcore/element.h>

// forward declaration
#ifndef PyObject_HEAD
struct _object;
typedef _object PyObject;
#endif

namespace akashi {
    namespace eval {

        class PythonObject;

        core::RenderProfile parse_renderProfile(core::borrowed_ptr<PythonObject> obj);

        core::AtomProfile parse_atomProfile(core::borrowed_ptr<const PythonObject> obj);

        core::LayerProfile parse_layerProfile(core::borrowed_ptr<const PythonObject> obj);

        template <typename T>
        core::FrameContext parse_frameContext(core::borrowed_ptr<T> obj);

        core::LayerContext parse_layerContext(core::borrowed_ptr<const PythonObject> obj);

        core::Fraction parse_second(core::borrowed_ptr<PythonObject> obj);

        core::Style parse_style(core::borrowed_ptr<PythonObject> obj);

    }
}
