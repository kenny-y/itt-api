#define NAPI_VERSION 3
#include <cstring>
#include <string>
#include <node_api.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include <ittnotify.h>

static std::string get_utf8_string(napi_env env, napi_value js_str) {

    size_t      len;
    size_t      dummy;
    char       *buf = 0;
    napi_status status;

    std::string result;
    status = napi_get_value_string_utf8(env, js_str, NULL, 0, &len);
    if (status == napi_ok) {
        buf = (char *) malloc(len + 1);
        status = napi_get_value_string_utf8(env, js_str, buf, len + 1, &dummy);
        if (status != napi_ok) {
            buf = 0;
        }
        result = buf;
        free(buf);
    }

    return result;
}

bool fetch_args(napi_env env, napi_callback_info info, napi_value* argv, size_t args_count) {
    size_t          argc = 1;
    napi_status     status;

    status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    if (status != napi_ok) {
        napi_throw_error(env, "EINVAL", "Call to napi_get_cb_info failed");
        return false;
    }

    if (argc != args_count) {
        std::string msg = "function expects only";
        msg += " ";
        msg += args_count;
        msg += " ";
        msg += "argument";
        napi_throw_error(env, "EINVAL", msg.c_str());
        return false;
    }

    return true;
}

inline std::string handle2str(void* ptr) {
    return std::to_string(reinterpret_cast<uint64_t>(ptr));
}

inline void* str2handle(const std::string& str) {
    return reinterpret_cast<void*>(std::stoull(str));
}

napi_value create_value(napi_env env, const std::string& str) {
    napi_value result;
    napi_status status = napi_create_string_utf8(env, str.c_str(), str.size(), &result);
    if (status != napi_ok) {
        napi_throw_error(env, "EINVAL", "Call to napi_create_string_utf8 failed");
        return NULL;
    }
    return result;
}

napi_value create_string_handle(napi_env env, napi_callback_info info) {
    napi_value argv[1];

    if (!fetch_args(env, info, argv, 1)) {
        return NULL;
    }

    std::string string_name = get_utf8_string(env, argv[0]);
    std::string string_handle("invalid_handle");

    if (string_name.size() > 0) {
        // Create a string handle
        __itt_string_handle* str = __itt_string_handle_create(string_name.c_str());
        string_handle = handle2str(str);
    }

    return create_value(env, string_handle);
}

napi_value task_begin(napi_env env, napi_callback_info info) {
    napi_value argv[2];

    if (!fetch_args(env, info, argv, 2)) {
        return NULL;
    }

    std::string domain_handle = get_utf8_string(env, argv[0]);
    std::string string_handle = get_utf8_string(env, argv[1]);

    if (string_handle.size() > 0 && domain_handle.size() > 0) {
        __itt_domain* domain = (__itt_domain*)str2handle(domain_handle);
        __itt_string_handle* task_name_handle = (__itt_string_handle*)str2handle(string_handle);
        __itt_task_begin(domain, __itt_null, __itt_null, task_name_handle);
    }

    return NULL;
}

napi_value task_end(napi_env env, napi_callback_info info) {
    napi_value argv[1];

    if (!fetch_args(env, info, argv, 1)) {
        return NULL;
    }

    std::string domain_handle = get_utf8_string(env, argv[0]);

    if (domain_handle.size() > 0) {
        __itt_domain* domain = (__itt_domain*)str2handle(domain_handle);
        __itt_task_end(domain);
    }

    return NULL;
}

napi_value create_domain(napi_env env, napi_callback_info info) {
    napi_value      argv[1];

    if (!fetch_args(env, info, argv, 1)) {
        return NULL;
    }

    std::string domain_name = get_utf8_string(env, argv[0]);
    std::string domain_handle("invalid_handle");
    if (domain_name.size() > 0) {
        // Create a domain that is visible globally
        __itt_domain* domain = __itt_domain_create(domain_name.c_str());
        domain_handle = handle2str(domain);
    }

    return create_value(env, domain_handle);
}

napi_status create_function(napi_env env, void* func, const char* name, napi_value exports) {
    napi_value  js_func;
    napi_status status;

    status = napi_create_function(env, NULL, 0, reinterpret_cast<napi_callback>(func), NULL, &js_func);
    if (status != napi_ok) {
        napi_throw_error(env, "EINVAL", "Cannot create JavaScript function from c function");
        return status;
    }

    status = napi_set_named_property(env, exports, name, js_func);
    if (status != napi_ok) {
        napi_throw_error(env, "EINVAL", "Cannot set module.exports");
        return status;
    }

    return napi_ok;
}

napi_value init (napi_env env, napi_value exports) {
    napi_status status = napi_ok;

    if (status == napi_ok) status = create_function(env, create_domain, "create_domain", exports);
    if (status == napi_ok) status = create_function(env, create_string_handle, "create_string_handle", exports);
    if (status == napi_ok) status = create_function(env, task_begin, "task_begin", exports);
    if (status == napi_ok) status = create_function(env, task_end, "task_end", exports);

    if (status != napi_ok) {
        return NULL;
    }

    return exports;
}

NAPI_MODULE(itt_api, init)
