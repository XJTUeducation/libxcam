/*
 * image_handler.h - image image handler class
 *
 *  Copyright (c) 2017 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Wind Yuan <feng.yuan@intel.com>
 */

#ifndef XCAM_IMAGE_HANDLER_H
#define XCAM_IMAGE_HANDLER_H

#include <xcam_std.h>
#include <meta_data.h>
#include <buffer_pool.h>
#include <worker.h>

#define DECLARE_HANDLER_CALLBACK(CbClass, Next, mem_func)                \
    class CbClass : public ::XCam::ImageHandler::Callback {              \
        private: ::XCam::SmartPtr<Next>  _h;                             \
        public: CbClass (const ::XCam::SmartPtr<Next> &h) { _h = h;}     \
        protected: void execute_status (                                 \
            const ::XCam::SmartPtr<::XCam::ImageHandler> &handler,       \
            const ::XCam::SmartPtr<::XCam::ImageHandler::Parameters> &params,  \
            const XCamReturn error) {                                    \
            _h->mem_func (handler, params, error);  }                    \
    }

#define XCAM_DEFAULT_HANDLER_BUF_CAP 4

namespace XCam {

class ImageHandler;

class ImageHandler
    : public RefObj
{
public:
    struct Parameters {
        SmartPtr<VideoBuffer> in_buf;
        SmartPtr<VideoBuffer> out_buf;

        Parameters (const SmartPtr<VideoBuffer> &in = NULL, const SmartPtr<VideoBuffer> &out = NULL)
            : in_buf (in), out_buf (out)
        {}
        virtual ~Parameters() {}
        bool add_meta (const SmartPtr<MetaBase> &meta);
        template <typename MType> SmartPtr<MType> find_meta ();

    private:
        MetaBaseList       _metas;
    };

    class Callback {
    public:
        Callback () {}
        virtual ~Callback () {}
        virtual void execute_status (
            const SmartPtr<ImageHandler> &handler, const SmartPtr<Parameters> &params, const XCamReturn error) = 0;

    private:
        XCAM_DEAD_COPY (Callback);
    };

public:
    explicit ImageHandler (const char* name);
    virtual ~ImageHandler ();

    bool set_callback (SmartPtr<Callback> cb) {
        _callback = cb;
        return true;
    }
    const SmartPtr<Callback> & get_callback () const {
        return _callback;
    }
    const char *get_name () const {
        return _name;
    }
    bool set_out_video_info (const VideoBufferInfo &info);
    bool enable_allocator (bool enable, uint32_t buf_count = XCAM_DEFAULT_HANDLER_BUF_CAP);

    // virtual functions
    // execute_buffer params should  NOT be const
    virtual XCamReturn execute_buffer (const SmartPtr<Parameters> &params, bool sync);
    virtual XCamReturn finish ();
    virtual XCamReturn terminate ();

protected:
    virtual XCamReturn configure_resource (const SmartPtr<Parameters> &param) = 0;
    virtual SmartPtr<BufferPool> create_allocator () = 0;
    virtual XCamReturn configure_rest ();
    virtual XCamReturn start_work (const SmartPtr<Parameters> &param) = 0;

    virtual void execute_status_check (const SmartPtr<Parameters> &params, const XCamReturn error);

    bool set_allocator (const SmartPtr<BufferPool> &allocator);
    const SmartPtr<BufferPool> &get_allocator () const {
        return _allocator;
    }
    XCamReturn reserve_buffers (const VideoBufferInfo &info, uint32_t count);
    SmartPtr<VideoBuffer> get_free_buf ();

    const VideoBufferInfo &get_out_video_info () {
        return _out_video_info;
    }

private:
    XCAM_DEAD_COPY (ImageHandler);

protected:
    bool                    _need_configure;
    bool                    _enable_allocator;

private:
    SmartPtr<Callback>      _callback;
    VideoBufferInfo         _out_video_info;
    SmartPtr<BufferPool>    _allocator;
    uint32_t                _buf_capacity;
    char                   *_name;
};

inline bool
ImageHandler::Parameters::add_meta (const SmartPtr<MetaBase> &meta)
{
    if (!meta.ptr ())
        return false;

    _metas.push_back (meta);
    return true;
}

template <typename MType>
SmartPtr<MType>
ImageHandler::Parameters::find_meta ()
{
    for (MetaBaseList::iterator i = _metas.begin (); i != _metas.end (); ++i) {
        SmartPtr<MType> m = (*i).dynamic_cast_ptr<MType> ();
        if (m.ptr ())
            return m;
    }
    return NULL;
}

};

#endif //XCAM_IMAGE_HANDLER_H
