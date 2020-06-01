#include <thread>
#include <memory>
#include "_call.h"
#include "_hash.h"
#include "_chrono_base.h"
#include "OmfMain.h"
#include "OmfObject.h"
#include "OmfDbg.h"
#include "OmfAttrSet.h"
#include "OmfHelper.h"
#include "IAudioSource.h"
////////////////////////////////////////////////////////////
#undef dbgEntryTest
#define dbgEntryTest(s) dbgEntrySky(s)
/////////////////////////////////////////////
using namespace omf;
using namespace omf::chrono;
using namespace omf::api;
using namespace omf::api::streaming;
using namespace omf::api::streaming::common;
////////////////////////////////////////////////////////////
static const char* _fname=0;
static int _seconds=30;
static const char* _keywords="dualos";
static const char* _codec=0;
static bool _aec=0;
static const char* _aecpara=0;
////////////////////////////////////////////
static bool _exit = false;
////////////////////////////////////////////
static OmfHelper::Item _options0[]{
	{"omfAudSrc(...): \n"
	 "This demo shows how to get audio streaming from OMF using IAudioSource interface.\n"
	 "  omfAudSrc -n test.pcm -d 30\n"
	 "  omfAudSrc -n test.aac -d 30 -c aac:br=128000\n"
	 "  omfAudSrc -n test.alaw -d 30 -c alaw\n"
	 "  omfAudSrc -n test.ulaw -d 30 -c ulaw\n"
	 "  omfAudSrc -n test.g722 -d 30 -c g722\n"
     "	omfAudSrc -n test.opus  -d 10 -c opus\n"
	},
	{"fname",'n', _fname		,"record filename(*.pcm)."},
	{"duration",'d', _seconds	,"process execute duration(*s)."},
	{"keywords",'k', _keywords	,"select the IAudioSource with keywords.Usually use the default values."},
	{"codec",'c', _codec		,"set the audio codec:eg.. aac:br=128000"},
	{"codec",'e', _aec		,"set the pcm aec enable"},
	{},
};
////////////////////////////////////////////
static bool MessageProcess(const char* msg0){
	dbgTestPSL(msg0);
	OmfAttrSet ap(msg0);
	returnIfErrC(true,!ap);
	auto msg = ap.Get("msg");
	returnIfErrC(true,!msg);
	switch(Hash(msg)){
		case Hash("error"):
		case Hash("err"):
			dbgNotePSL("error");
			_exit=true;
			break;
		default:
			dbgTestPSL("unknow message:"<<msg);
			break;
	}
	return true;
}
static bool ProcessPull(IAudioSource*src,FILE*fd){
	//start streaming
	returnIfErrC(false,!src->ChangeUp(State::play));
	//streaming....
	auto end = Now()+Seconds(_seconds);
	while(!_exit && Now()<end) {
		std::shared_ptr<IAudioSource::frame_t> frm;
		returnIfErrCS(false, !src->PullFrame(frm), "pull frame fail!");
		if(!frm->data || !frm->size)
			continue;
		///
		dbgTestPSL(frm->index
						   <<','<<frm->data
						   <<','<<frm->size
						   <<','<<frm->iskeyframe
						   <<','<<frm->pts
		);
		dbgTestDL(frm->data,16);
		///write to file
		if(fd)fwrite(frm->data,1,frm->size,fd);
		///sleep & trigger
		auto interval = 10_ms;
		//if(src->IsSupportSingleFrameTrigger()) {
		//	interval = Seconds(_triggerInterval);
		//	src->Trigger();
		//}
		std::this_thread::sleep_for(interval);
	}
	//stop streaming
	returnIfErrC(false,!src->ChangeDown(State::ready));
	return true;
}
static bool ProcessPush(IAudioSource*src,FILE*fd){
	//set push callback
	src->RegisterOutputCallback([&fd](std::shared_ptr<IAudioSource::frame_t>&frm){
		dbgTestPSL(frm->index
						   <<','<<frm->data
						   <<','<<frm->size
						   <<','<<frm->iskeyframe
						   <<','<<frm->pts
		);
		dbgTestDL(frm->data,16);
		///
		if(_codec && !strcmp(_codec,"opus")){
			uint32 framesize = frm->size;
			if(fd)fwrite(&framesize,1,4,fd);
		}

		if(fd)fwrite(frm->data,1,frm->size,fd);
		return true;
	});
	//start streaming
	returnIfErrC(false,!src->ChangeUp(State::play));
	//streaming...
	auto end = Now()+Seconds(_seconds);
	while(!_exit && Now()<end) {
		///sleep & trigger
		auto interval = 10_ms;
		//if(src->IsSupportSingleFrameTrigger()) {
		//	interval = Seconds(_triggerInterval);
		//	src->Trigger();
		//}
		std::this_thread::sleep_for(interval);
	}
	//stop streaming
	returnIfErrC(false,!src->ChangeDown(State::ready));
	return true;
}
static bool Process(bool _dbg){dbgTestPL();
	///////////////////////////////////////
	//create a IAudioSource instance with keywords.
	dbgTestPVL(_keywords);
	std::unique_ptr<IAudioSource> src(IAudioSource::CreateNew(_keywords));
	returnIfErrC(false,!src);
	//set audio srouce parameters
	src->SetCodec(_codec);

	src->SetAEC(_aec,_aecpara);

	//open streaming
	returnIfErrC(false,!src->ChangeUp(State::ready));
	//get streaming parameters after Open().
	auto info = src->GetAudioMediaInfo();
	dbgTestPVL(info.rate);
	dbgTestPVL(info.channels);
	dbgTestPVL(info.media);

	////////////////////////////////////////////////////////
	FILE* fd=0;
	if(_fname) {
		fd = fopen(_fname, "wb");
		if (!fd) {
			dbgErrPSL("open file fail:" << _fname);
		}
	}
	ExitCall ecfd([fd](){if(fd)fclose(fd);});
	//////////////////////////////////
	//streaming......
	//ProcessMic(src.get());
	if(src->IsSupportedPullFrame()){
		returnIfErrC(false,!ProcessPull(src.get(),fd));
	}else if(src->IsSupportedOutputFrameCallback()){
		returnIfErrC(false,!ProcessPush(src.get(),fd));
	}else{
		dbgErrPSL("null support output mode.");
	}
	returnIfErrC(false,!src->ChangeDown(State::null));
	return true;
}
////////////////////////////////////////////
static bool Check(){
	return true;
}
////////////////////////////////
int main(int argc,char* argv[]){
	dbgNotePSL("omfAudSrc(...)\n");
	///parse the input params
	OmfHelper helper(_options0,argc,argv);dbgTestPL();
	///--help
	returnIfTestC(0,!helper);dbgTestPL();
	///check the params
	returnIfErrC(0,!Check());dbgTestPL();
	///init the omf module
	OmfMain omf;dbgTestPL();
	omf.Helper(helper);dbgTestPL();
	///process
	Process(helper.Debug());dbgTestPL();
	///
	return 0;
}

