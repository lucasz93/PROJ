#include "proj.h"
#include "proj_internal.h"

/*****************************************************************************/
void pj_scan_recursive(PJ* P, PJscan& s) {
/*****************************************************************************
    Finds all possible functions used by this projection.
******************************************************************************/
    pj_scan_local(P, s);
    P->axisswap->host->scan(P->axisswap, s);
    P->cart->host->scan(P->cart, s);
    P->cart_wgs84->host->scan(P->cart_wgs84, s);
    P->helmert->host->scan(P->helmert, s);
    P->hgridshift->host->scan(P->hgridshift, s);
    P->vgridshift->host->scan(P->vgridshift, s);
}

/*****************************************************************************/
void pj_scan_local(PJ* P, PJscan& s) {
/*****************************************************************************
    Finds all the functions used by this operation, and records them so we can
    populate the OpenCL double dispatch table for this projection.
******************************************************************************/
    bool add_file = false;

    if (P->fwd[0])
    {
        s.functions.insert(P->fwd);
        add_file = true;
    }

    if (P->inv[0])
    {
        s.functions.insert(P->inv);
        add_file = true;
    }

    if (P->fwd3d[0])
    {
        s.functions.insert(P->fwd3d);
        add_file = true;
    }

    if (P->inv3d[0])
    {
        s.functions.insert(P->inv3d);
        add_file = true;
    }

    if (P->fwd4d[0])
    {
        s.functions.insert(P->fwd4d);
        add_file = true;
    }

    if (P->inv4d[0])
    {
        s.functions.insert(P->inv4d);
        add_file = true;
    }

    if (add_file)
    {
        s.files.insert(P->host->file);
    }
}

/*****************************************************************************/
void pj_scan_nop(PJ* P, PJscan& s) {
/*****************************************************************************/
    (void) P;
    (void) s;
}

#ifdef PROJ_OPENCL

//
// TOOD: a function to write the kernel using PJscan.
//

static PJ_XY pj_double_dispatch_fwd(PJ_LP lp, PJ *P, const char *name)
{
    // These should be defined by the PROJECTION() macro.
    // Since the projections define their kernels as static methods, we need to include their source files anyway.
    // The generated source file should be something like:

#if 0
#include <proj_internal.h>
#include <proj/src/projections/eqc.cpp>

#ifndef PROJ_DOUBLE_DISPATCH
#define PROJ_DOUBLE_DISPATCH
static PJ_XY pj_double_dispatch_fwd(PJ_LP lp, PJ *P, const char *name)
{
    * This method is autogenerated - need to recurse the projection looking for any valid P->fwds.
        * Need to have a new host function pointer - 'scan'.
            * This will default to searching P->[fwd|inv|axisswap|helmert|etc].
            * Can overload for more complex types, such as pipeline.cpp.
            * If any kernel functions are found - we also store the filename using the __FILE__ macro.
    * Also need to do similarly for inv, fwd3d, inv3d, fwd4d, inv4d.
    * There's an opportunity here to use hashes instead of strings (and switch instead of if-elseif), but that can come next.
}
#endif

#include <proj/src/fwd.cpp>
#include <proj/src/inv.cpp>
#include <proj/src/shared.cpp>
    // Defines things like proj_trans, and other misc things.

#endif

#ifdef PROJ_OPENCL_eqc
    if (!strcmp(name, "eqc_s_forward"))
    {
        return eqc_s_forward(lp, P);
    }
#endif
}

#endif