#pragma once

// IT IS ADVISED TO BE INCLUDED BEFORE ANY QT INCLUDES TO MINIMIZE DAMAGE.

// There is a function named emit in oneapi/tbb/profiling.h, which conflicts with the
// Qt's emit keyword. There are lots of solutions for that described at
// https://github.com/uxlfoundation/oneTBB/issues/547
#ifndef Q_MOC_RUN
    #if defined(emit)
        #undef emit
            #include <tbb/tbb.h>
        #define emit // restore the macro definition of "emit", as it was defined in gtmetamacros.h
    #else
        #include <tbb/tbb.h>
    #endif // defined(emit)
#endif // Q_MOC_RUN
