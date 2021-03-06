#include <node.h>
#include <iostream>
#include <node_buffer.h>
#include "Recognizer.h"

using namespace v8;
using namespace std;

Recognizer::Recognizer() {

}

Recognizer::~Recognizer() {
    ps_free(ps);
}

Persistent<Function> Recognizer::constructor;

void Recognizer::Init(Handle<Object> exports) {
    Isolate* isolate = Isolate::GetCurrent();

    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
    tpl->SetClassName(String::NewFromUtf8(isolate,"Recognizer"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    //tpl->Set(String::NewFromUtf8(isolate, "fromFloat"), FunctionTemplate::New(isolate, FromFloat)->GetFunction());
    tpl->Set(String::NewFromUtf8(isolate, "modelDirectory"), String::NewFromUtf8(isolate, MODELDIR));
    tpl->PrototypeTemplate()->SetAccessor(String::NewFromUtf8(isolate, "search"), GetSearch, SetSearch);
    NODE_SET_PROTOTYPE_METHOD(tpl, "start", Start);
    NODE_SET_PROTOTYPE_METHOD(tpl, "stop", Stop);
    NODE_SET_PROTOTYPE_METHOD(tpl, "restart", Restart);
    NODE_SET_PROTOTYPE_METHOD(tpl, "fromFloat", FromFloat);
    NODE_SET_PROTOTYPE_METHOD(tpl, "addKeyphraseSearch", AddKeyphraseSearch);
    NODE_SET_PROTOTYPE_METHOD(tpl, "addKeywordsSearch", AddKeywordsSearch);
    NODE_SET_PROTOTYPE_METHOD(tpl, "addGrammarSearch", AddGrammarSearch);
    NODE_SET_PROTOTYPE_METHOD(tpl, "addNgramSearch", AddNgramSearch);
    //NODE_SET_PROTOTYPE_METHOD(tpl, "search", Search);
    NODE_SET_PROTOTYPE_METHOD(tpl, "write", Write);
    NODE_SET_PROTOTYPE_METHOD(tpl, "writeSync", WriteSync);
    /*
    // Static Methods and Properties
    tpl->Set(String::NewSymbol("fromFloat"), FunctionTemplate::New(FromFloat)->GetFunction());
    tpl->Set(String::NewSymbol("modelDirectory"), String::NewSymbol(MODELDIR));

    // Prototype Methods and Properies
    tpl->PrototypeTemplate()->Set(String::NewSymbol("start"), FunctionTemplate::New(Start)->GetFunction());
    tpl->PrototypeTemplate()->Set(String::NewSymbol("stop"), FunctionTemplate::New(Stop)->GetFunction());
    tpl->PrototypeTemplate()->Set(String::NewSymbol("restart"), FunctionTemplate::New(Restart)->GetFunction());

    tpl->PrototypeTemplate()->Set(String::NewSymbol("addKeyphraseSearch"), FunctionTemplate::New(AddKeyphraseSearch)->GetFunction());
    tpl->PrototypeTemplate()->Set(String::NewSymbol("addKeywordsSearch"), FunctionTemplate::New(AddKeywordsSearch)->GetFunction());
    tpl->PrototypeTemplate()->Set(String::NewSymbol("addGrammarSearch"), FunctionTemplate::New(AddGrammarSearch)->GetFunction());
    tpl->PrototypeTemplate()->Set(String::NewSymbol("addNgramSearch"), FunctionTemplate::New(AddNgramSearch)->GetFunction());


    tpl->PrototypeTemplate()->Set(String::NewSymbol("write"), FunctionTemplate::New(Write)->GetFunction());
    tpl->PrototypeTemplate()->Set(String::NewSymbol("writeSync"), FunctionTemplate::New(WriteSync)->GetFunction());
    */
    constructor.Reset(isolate, tpl->GetFunction());
    exports->Set(String::NewFromUtf8(isolate, "Recognizer"), tpl->GetFunction());
}

void Recognizer::New(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    if(!args.IsConstructCall()) {
        const int argc = 2;
        Local<Value> argv[argc] = { args[0], args[1] };
        Local<Function> cons = Local<Function>::New(isolate, constructor);
        args.GetReturnValue().Set(cons->NewInstance(argc, argv));
    }

    if(args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate,"Incorrect number of arguments, expected options and callback")));
        args.GetReturnValue().Set(Undefined(isolate));
    }

    if(!args[0]->IsObject()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate,"Expected options to be an object")));
        args.GetReturnValue().Set(Undefined(isolate));
    }

    if(!args[1]->IsFunction()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate,"Expected callback to be a function")));
        args.GetReturnValue().Set(Undefined(isolate));
    }

    Recognizer* instance = new Recognizer();

    Handle<Object> options = args[0]->ToObject();
    Local<Function> cb = Local<Function>::Cast(args[1]);
    instance->callback.Reset(isolate, cb);

    String::Utf8Value hmmValue(Default(options->Get(String::NewFromUtf8(isolate,"hmm")), String::NewFromUtf8(isolate,MODELDIR "/en-us/en-us")));
    String::Utf8Value dictValue(Default(options->Get(String::NewFromUtf8(isolate,"dict")), String::NewFromUtf8(isolate,MODELDIR "/en-us/cmudict-en-us.dict")));
    String::Utf8Value samprateValue(Default(options->Get(String::NewFromUtf8(isolate,"samprate")), String::NewFromUtf8(isolate,"44100")));
    String::Utf8Value nfftValue(Default(options->Get(String::NewFromUtf8(isolate,"nfft")), String::NewFromUtf8(isolate,"2048")));

    cmd_ln_t* config = cmd_ln_init(NULL, ps_args(), TRUE,
            "-hmm", *hmmValue,
            "-dict", *dictValue,
            "-samprate", *samprateValue,
            "-nfft", *nfftValue,
            NULL);

    instance->ps = ps_init(config);

    instance->Wrap(args.Holder());

    args.GetReturnValue().Set(args.Holder());
}

void Recognizer::AddKeyphraseSearch(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.Holder());

    if(args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate,"Incorrect number of arguments, expected name and keyphrase")));
        args.GetReturnValue().Set(args.Holder());
    }

    if(!args[0]->IsString() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate,"Expected both name and keyphrase to be strings")));
        args.GetReturnValue().Set(args.Holder());
    }

    String::Utf8Value name(args[0]);
    String::Utf8Value keyphrase(args[1]);

    int result = ps_set_keyphrase(instance->ps, *name, *keyphrase);
    if(result < 0)
        isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate,"Failed to add keyphrase search to recognizer")));

    args.GetReturnValue().Set(args.Holder());
}

void Recognizer::AddKeywordsSearch(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.Holder());

    if(args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate,"Incorrect number of arguments, expected name and file")));
        args.GetReturnValue().Set(args.Holder());
    }

    if(!args[0]->IsString() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate,"Expected both name and file to be strings")));
        args.GetReturnValue().Set(args.Holder());
    }

    String::Utf8Value name(args[0]);
    String::Utf8Value file(args[1]);

    int result = ps_set_kws(instance->ps, *name, *file);
    if(result < 0)
        isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate,"Failed to add keywords search to recognizer")));

    args.GetReturnValue().Set(args.Holder());
}

void Recognizer::AddGrammarSearch(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.Holder());

    if(args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate,"Incorrect number of arguments, expected name and file")));
        args.GetReturnValue().Set(args.Holder());
    }

    if(!args[0]->IsString() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate,"Expected both name and file to be strings")));
        args.GetReturnValue().Set(args.Holder());
    }

    String::Utf8Value name(args[0]);
    String::Utf8Value file(args[1]);

    int result = ps_set_jsgf_file(instance->ps, *name, *file);
    if(result < 0)
        isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate,"Failed to add grammar search to recognizer")));

    args.GetReturnValue().Set(args.Holder());
}

void Recognizer::AddNgramSearch(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.Holder());

    if(args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate,"Incorrect number of arguments, expected name and file")));
        args.GetReturnValue().Set(args.Holder());
    }

    if(!args[0]->IsString() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate,"Expected both name and file to be strings")));
        args.GetReturnValue().Set(args.Holder());
    }

    String::Utf8Value name(args[0]);
    String::Utf8Value file(args[1]);

    int result = ps_set_lm_file(instance->ps, *name, *file);
    if(result < 0)
        isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate,"Failed to add Ngram search to recognizer")));

    args.GetReturnValue().Set(args.Holder());
}

void Recognizer::GetSearch(Local<String> property, const PropertyCallbackInfo<Value>& args) {
    Isolate* isolate = Isolate::GetCurrent();
    Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.This());

    Local<Value> search = String::NewFromUtf8(isolate,ps_get_search(instance->ps));

    args.GetReturnValue().Set(search);
}

void Recognizer::SetSearch(Local<String> property, Local<Value> value, const PropertyCallbackInfo<void>& args) {
    Isolate* isolate = Isolate::GetCurrent();
    Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.This());

    String::Utf8Value search(value);

    ps_set_search(instance->ps, *search);

    args.GetReturnValue().Set(args.Holder());
}

void Recognizer::Start(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = Isolate::GetCurrent();
    Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.Holder());

    int result = ps_start_utt(instance->ps);
    if(result)
        isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate,"Failed to start PocketSphinx processing")));

    args.GetReturnValue().Set(args.Holder());
}

void Recognizer::Stop(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = Isolate::GetCurrent();
    Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.Holder());

    int result = ps_end_utt(instance->ps);
    if(result)
        isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate,"Failed to end PocketSphinx processing")));

    args.GetReturnValue().Set(args.Holder());
}

void Recognizer::Restart(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = Isolate::GetCurrent();
    Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.Holder());

    int result = ps_start_utt(instance->ps);
    if(result)
        isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate,"Failed to start PocketSphinx processing")));

    result = ps_end_utt(instance->ps);
    if(result)
        isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate,"Failed to restart PocketSphinx processing")));

    args.GetReturnValue().Set(args.Holder());
}

void Recognizer::Write(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = Isolate::GetCurrent();
    Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.Holder());

    Handle<Object> buffer = args[0]->ToObject();

    if(!args.Length()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate,"Expected a data buffer to be provided")));
        args.GetReturnValue().Set(args.Holder());
    }

    if(!node::Buffer::HasInstance(buffer)) {
        Local<Value> argv[1] = { Exception::Error(String::NewFromUtf8(isolate,"Expected data to be a buffer")) };
        Local<Function> cb = Local<Function>::New(isolate, instance->callback);
        cb->Call(isolate->GetCurrentContext()->Global(), 1, argv);
        args.GetReturnValue().Set(args.Holder());
    }

    AsyncData* data = new AsyncData();
    data->instance = instance;
    data->data = (int16*) node::Buffer::Data(buffer);
    data->length = node::Buffer::Length(buffer) / sizeof(int16);

    uv_work_t* req = new uv_work_t();
    req->data = data;

    uv_queue_work(uv_default_loop(), req, AsyncWorker, (uv_after_work_cb)AsyncAfter);

    args.GetReturnValue().Set(args.Holder());
}

void Recognizer::WriteSync(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.Holder());

    Handle<Object> buffer = args[0]->ToObject();

    if(!args.Length()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate,"Expected a data buffer to be provided")));
        return;
    }

    if(!node::Buffer::HasInstance(buffer)) {
        Local<Value> argv[1] = { Exception::Error(String::NewFromUtf8(isolate,"Expected data to be a buffer")) };
        Local<Function> cb = Local<Function>::New(isolate, instance->callback);
        cb->Call(isolate->GetCurrentContext()->Global(), 1, argv);
        return;
    }

    int16* data = (int16*) node::Buffer::Data(buffer);
    size_t length = node::Buffer::Length(buffer) / sizeof(int16);

    if(ps_process_raw(instance->ps, data, length, FALSE, FALSE) < 0) {
        cout << "Error3" << endl;
        Handle<Value> argv[1] = { Exception::Error(String::NewFromUtf8(isolate,"Failed to process audio data")) };
        Local<Function> cb = Local<Function>::New(isolate, instance->callback);
        cb->Call(isolate->GetCurrentContext()->Global(), 1, argv);
        args.GetReturnValue().Set(args.Holder());
    }

    int32 score;
    const char* hyp = ps_get_hyp(instance->ps, &score);

    Handle<Value> argv[3] = { Null(isolate), hyp ? String::NewFromUtf8(isolate,hyp) : String::NewFromUtf8(isolate, ""), NumberObject::New(isolate,score)};
    Local<Function> cb = Local<Function>::New(isolate, instance->callback);
    cb->Call(isolate->GetCurrentContext()->Global(), 3, argv);

    args.GetReturnValue().Set(args.Holder());
}

void Recognizer::AsyncWorker(uv_work_t* request) {
    Isolate* isolate = Isolate::GetCurrent();
    AsyncData* data = reinterpret_cast<AsyncData*>(request->data);

    if(ps_process_raw(data->instance->ps, data->data, data->length, FALSE, FALSE)) {
        data->hasException = TRUE;
        data->exception = Exception::Error(String::NewFromUtf8(isolate,"Failed to process audio data"));
        return;
    }

    int32 score;
    const char* hyp = ps_get_hyp(data->instance->ps, &score);

    data->score = score;
    data->hyp = hyp;
}

void Recognizer::AsyncAfter(uv_work_t* request) {
    Isolate* isolate = Isolate::GetCurrent();
    AsyncData* data = reinterpret_cast<AsyncData*>(request->data);


    if(data->hasException) {
        Handle<Value> argv[1] = { data->exception };
        Local<Function> cb = Local<Function>::New(isolate, data->instance->callback);
        cb->Call(isolate->GetCurrentContext()->Global(), 1, argv);
    } else {
        Handle<Value> argv[3] = { Null(isolate), data->hyp ? String::NewFromUtf8(isolate,data->hyp) : String::NewFromUtf8(isolate, ""), NumberObject::New(isolate,data->score)};
        Local<Function> cb = Local<Function>::New(isolate, data->instance->callback);
        cb->Call(isolate->GetCurrentContext()->Global(), 3, argv);
    }
}

Local<Value> Recognizer::Default(Local<Value> value, Local<Value> fallback) {
    if(value->IsUndefined()) return fallback;
    return value;
}

void Recognizer::FromFloat(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    if(!args.Length()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate,"Expected a data buffer to be provided")));
        args.GetReturnValue().Set(args.Holder());
    }

    if(!node::Buffer::HasInstance(args[0])) {
        isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate,"Expected data to be a buffer")));
        args.GetReturnValue().Set(args.Holder());
    }

    float* data = reinterpret_cast<float*>(node::Buffer::Data(args[0]));
    size_t length = node::Buffer::Length(args[0]) / sizeof(float);

    Local<Object> slowBuffer = node::Buffer::New(length * sizeof(int16));
    int16* slowBufferData = reinterpret_cast<int16*>(node::Buffer::Data(slowBuffer));

    //args.GetReturnValue().Set(args.Holder());

    for(size_t i = 0; i < length; i++)
        slowBufferData[i] = data[i] * 32768;

    // Courtesy of http://sambro.is-super-awesome.com/2011/03/03/creating-a-proper-buffer-in-a-node-c-addon/
    Local<Object> globalObj = isolate->GetCurrentContext()->Global();
    Local<Function> bufferConstructor = Local<Function>::Cast(globalObj->Get(String::NewFromUtf8(isolate, "Buffer")));
    Handle<Value> constructorArgs[3] = { slowBuffer, Integer::New(isolate, length), Integer::New(isolate, 0) };
    Local<Object> actualBuffer = bufferConstructor->NewInstance(3, constructorArgs);
    args.GetReturnValue().Set(actualBuffer);
}
