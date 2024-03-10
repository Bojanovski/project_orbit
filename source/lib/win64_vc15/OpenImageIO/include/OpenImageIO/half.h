// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/


#pragma once

// Determine which Imath we're dealing with and include the appropriate path
// to half.h.

#if defined(__has_include) && __has_include(<Imath/half.h>)
#    include <Imath/half.h>
#elif 3 >= 3
#    include <Imath/half.h>
#else
#    include <OpenEXR/half.h>
#endif