/******************************************************************************
 * Project:  PROJ.4
 * Purpose:  Implementation of the PJ_CONTEXT thread context object.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2010, Frank Warmerdam
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/
#ifndef FROM_PROJ_CPP
#define FROM_PROJ_CPP
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <new>

#include "proj_experimental.h"
#include "proj_internal.h"
#include "filemanager.hpp"
#include "proj/internal/io_internal.hpp"

/************************************************************************/
/*                             pj_get_ctx()                             */
/************************************************************************/

PJ_CONTEXT* pj_get_ctx( PJ *pj )

{
    if (nullptr==pj)
        return pj_get_default_ctx ();
    if (nullptr==pj->host->ctx)
        return pj_get_default_ctx ();
    return pj->host->ctx;
}

/************************************************************************/
/*                        proj_assign_context()                         */
/************************************************************************/

/** \brief Re-assign a context to a PJ* object.
 *
 * This may be useful if the PJ* has been created with a context that is
 * thread-specific, and is later used in another thread. In that case,
 * the user may want to assign another thread-specific context to the
 * object.
 */
void proj_assign_context( PJ* pj, PJ_CONTEXT *ctx )
{
    if (pj==nullptr)
        return;
    pj->host->ctx = ctx;
    pj->shared_ctx = ctx ? ctx->shared : nullptr;
    if( pj->host->reassign_context )
    {
        pj->host->reassign_context(pj, ctx);
    }
    for( const auto &alt: pj->host->alternativeCoordinateOperations )
    {
        proj_assign_context(alt.pj, ctx);
    }

}

/************************************************************************/
/*                          createDefault()                             */
/************************************************************************/

/**********************************************************************/
static void default_map(void* user, void* ptr, int map)
/**********************************************************************/
{
    (void)user;
    (void)ptr;
    (void)map;
}

/**********************************************************************/
static void* default_malloc(void* user, size_t sz)
/**********************************************************************/
{
    (void)user;
    return malloc(sz);
}

/**********************************************************************/
static void* default_calloc(void* user, size_t n, size_t sz)
/**********************************************************************/
{
    (void)user;
    return calloc(n, sz);
}

/**********************************************************************/
static void default_free(void* user, void* ptr)
/**********************************************************************/
{
    if (ptr)
    {
        (void)user;
        free(ptr);
    }
}

pj_ctx pj_ctx::createDefault(pj_allocator *allocator)
{
    static pj_allocator defaultAllocator{ nullptr, default_malloc, default_calloc, default_free, default_map };

    if (nullptr==allocator)
        allocator = &defaultAllocator;

    pj_ctx ctx(allocator);
    ctx.debug_level = PJ_LOG_ERROR;
    ctx.logger = pj_stderr_logger;
    NS_PROJ::FileManager::fillDefaultNetworkInterface(&ctx);

    const char* projDebug = getenv("PROJ_DEBUG");
    if( projDebug != nullptr )
    {
        const int debugLevel = atoi(projDebug);
        if( debugLevel >= -PJ_LOG_TRACE )
            ctx.debug_level = debugLevel;
        else
            ctx.debug_level = PJ_LOG_TRACE;
    }

    return ctx;
}

/**************************************************************************/
/*                           get_cpp_context()                            */
/**************************************************************************/

projCppContext* pj_ctx::get_cpp_context()
{
    if (cpp_context == nullptr) {
        cpp_context = new projCppContext(this);
    }
    return cpp_context;
}

/************************************************************************/
/*                           set_search_paths()                         */
/************************************************************************/

void pj_ctx::set_search_paths(const std::vector<std::string>& search_paths_in )
{
    search_paths = search_paths_in;
    delete[] c_compat_paths;
    c_compat_paths = nullptr;
    if( !search_paths.empty() ) {
        c_compat_paths = new const char*[search_paths.size()];
        for( size_t i = 0; i < search_paths.size(); ++i ) {
            c_compat_paths[i] = search_paths[i].c_str();
        }
    }
}

/**************************************************************************/
/*                           set_ca_bundle_path()                         */
/**************************************************************************/

void pj_ctx::set_ca_bundle_path(const std::string& ca_bundle_path_in)
{
    ca_bundle_path = ca_bundle_path_in;
}

/************************************************************************/
/*                  pj_ctx(PJ_CONTEXT *ctx)                             */
/************************************************************************/

pj_ctx::pj_ctx(pj_allocator *a) :
    allocator(a)
{
    shared = allocator->svm_new<pj_ctx_shared>();
}

/************************************************************************/
/*                  pj_ctx(const pj_ctx& other)                   */
/************************************************************************/

pj_ctx::pj_ctx(const pj_ctx& other) :
    pj_ctx(other, other.allocator)
{}

pj_ctx::pj_ctx(const pj_ctx& other, pj_allocator *a) :
    allocator(a),
    debug_level(other.debug_level),
    logger(other.logger),
    logger_app_data(other.logger_app_data),
    cpp_context(other.cpp_context ? other.cpp_context->clone(this) : nullptr),
    use_proj4_init_rules(other.use_proj4_init_rules),
    epsg_file_exists(other.epsg_file_exists),
    ca_bundle_path(other.ca_bundle_path),
    env_var_proj_lib(other.env_var_proj_lib),
    file_finder(other.file_finder),
    file_finder_user_data(other.file_finder_user_data),
    custom_sqlite3_vfs_name(other.custom_sqlite3_vfs_name),
    user_writable_directory(other.user_writable_directory),
    // BEGIN ini file settings
    iniFileLoaded(other.iniFileLoaded),
    endpoint(other.endpoint),
    networking(other.networking),
    gridChunkCache(other.gridChunkCache),
    defaultTmercAlgo(other.defaultTmercAlgo)
    // END ini file settings
{
    set_search_paths(other.search_paths);

    shared = allocator->svm_new<pj_ctx_shared>();

    *shared = *other.shared;
}

/************************************************************************/
/*                         pj_get_default_ctx()                         */
/************************************************************************/

PJ_CONTEXT* pj_get_default_ctx()

{
    // C++11 rules guarantee a thread-safe instantiation.
    static pj_ctx default_context(pj_ctx::createDefault(nullptr));
    return &default_context;
}

/************************************************************************/
/*                            ~pj_ctx()                              */
/************************************************************************/

pj_ctx::~pj_ctx()
{
    delete[] c_compat_paths;
    proj_context_delete_cpp_context(cpp_context);

    allocator->svm_delete(shared);
}

/************************************************************************/
/*                            proj_context_clone()                      */
/*           Create a new context based on a custom context             */
/************************************************************************/

PJ_CONTEXT* proj_context_clone (PJ_CONTEXT *ctx)
{
    if (nullptr==ctx)
        return proj_context_create();

    return new (std::nothrow) pj_ctx(*ctx);
}


