
    
    int portaudio_import();


// ##########################################################################################################################
// ######## generated from portaudio.h ######################################################################################
// ##########################################################################################################################


int (*Pa_GetVersion)( void );  //////////////////////////////////////////////////////////////////////////////////// 
const char* (*Pa_GetVersionText)( void );  //////////////////////////////////////////////////////////////////////////////////// 

#define paMakeVersionNumber(major, minor, subminor) (((major)&0xFF)<<16 | ((minor)&0xFF)<<8 | ((subminor)&0xFF))

typedef struct PaVersionInfo {
    int versionMajor;
    int versionMinor;
    int versionSubMinor;
    const char *versionControlRevision;
    const char *versionText;
} PaVersionInfo;

const PaVersionInfo* (*Pa_GetVersionInfo)( void );  //////////////////////////////////////////////////////////////////////////////////// 

typedef int PaError;
typedef enum PaErrorCode {
    paNoError = 0,
    paNotInitialized = -10000,
    paUnanticipatedHostError,
    paInvalidChannelCount,
    paInvalidSampleRate,
    paInvalidDevice,
    paInvalidFlag,
    paSampleFormatNotSupported,
    paBadIODeviceCombination,
    paInsufficientMemory,
    paBufferTooBig,
    paBufferTooSmall,
    paNullCallback,
    paBadStreamPtr,
    paTimedOut,
    paInternalError,
    paDeviceUnavailable,
    paIncompatibleHostApiSpecificStreamInfo,
    paStreamIsStopped,
    paStreamIsNotStopped,
    paInputOverflowed,
    paOutputUnderflowed,
    paHostApiNotFound,
    paInvalidHostApi,
    paCanNotReadFromACallbackStream,
    paCanNotWriteToACallbackStream,
    paCanNotReadFromAnOutputOnlyStream,
    paCanNotWriteToAnInputOnlyStream,
    paIncompatibleStreamHostApi,
    paBadBufferPtr
} PaErrorCode;

const char* (*Pa_GetErrorText)( PaError errorCode );  //////////////////////////////////////////////////////////////////////////////////// 
PaError (*Pa_Initialize)( void );  //////////////////////////////////////////////////////////////////////////////////// 
PaError (*Pa_Terminate)( void );  //////////////////////////////////////////////////////////////////////////////////// 

typedef int PaDeviceIndex;
#define paNoDevice ((PaDeviceIndex)-1)
#define paUseHostApiSpecificDeviceSpecification ((PaDeviceIndex)-2)
typedef int PaHostApiIndex;

PaHostApiIndex (*Pa_GetHostApiCount)( void );  //////////////////////////////////////////////////////////////////////////////////// 
PaHostApiIndex (*Pa_GetDefaultHostApi)( void );  //////////////////////////////////////////////////////////////////////////////////// 

typedef enum PaHostApiTypeId {
    paInDevelopment=0,
    paDirectSound=1,
    paMME=2,
    paASIO=3,
    paSoundManager=4,
    paCoreAudio=5,
    paOSS=7,
    paALSA=8,
    paAL=9,
    paBeOS=10,
    paWDMKS=11,
    paJACK=12,
    paWASAPI=13,
    paAudioScienceHPI=14,
    paAudioIO=15
} PaHostApiTypeId;

typedef struct PaHostApiInfo {
    int structVersion;
    PaHostApiTypeId type;
    const char *name;
    int deviceCount;
    PaDeviceIndex defaultInputDevice;
    PaDeviceIndex defaultOutputDevice;
} PaHostApiInfo;

const PaHostApiInfo* (*Pa_GetHostApiInfo)( PaHostApiIndex hostApi );  //////////////////////////////////////////////////////////////////////////////////// 
PaHostApiIndex (*Pa_HostApiTypeIdToHostApiIndex)( PaHostApiTypeId type );  //////////////////////////////////////////////////////////////////////////////////// 
PaDeviceIndex (*Pa_HostApiDeviceIndexToDeviceIndex)( PaHostApiIndex hostApi, int hostApiDeviceIndex );  //////////////////////////////////////////////////////////////////////////////////// 

typedef struct PaHostErrorInfo{
    PaHostApiTypeId hostApiType;
    long errorCode;
    const char *errorText;
}PaHostErrorInfo;

const PaHostErrorInfo* (*Pa_GetLastHostErrorInfo)( void );  //////////////////////////////////////////////////////////////////////////////////// 
PaDeviceIndex (*Pa_GetDeviceCount)( void );  //////////////////////////////////////////////////////////////////////////////////// 
PaDeviceIndex (*Pa_GetDefaultInputDevice)( void );  //////////////////////////////////////////////////////////////////////////////////// 
PaDeviceIndex (*Pa_GetDefaultOutputDevice)( void );  //////////////////////////////////////////////////////////////////////////////////// 

typedef double PaTime;
typedef unsigned long PaSampleFormat;

#define paFloat32        ((PaSampleFormat) 0x00000001)
#define paInt32          ((PaSampleFormat) 0x00000002)
#define paInt24          ((PaSampleFormat) 0x00000004)
#define paInt16          ((PaSampleFormat) 0x00000008)
#define paInt8           ((PaSampleFormat) 0x00000010)
#define paUInt8          ((PaSampleFormat) 0x00000020)
#define paCustomFormat   ((PaSampleFormat) 0x00010000)
#define paNonInterleaved ((PaSampleFormat) 0x80000000)

typedef struct PaDeviceInfo {
    int structVersion;
    const char *name;
    PaHostApiIndex hostApi;
    int maxInputChannels;
    int maxOutputChannels;
    PaTime defaultLowInputLatency;
    PaTime defaultLowOutputLatency;
    PaTime defaultHighInputLatency;
    PaTime defaultHighOutputLatency;
    double defaultSampleRate;
} PaDeviceInfo;

const PaDeviceInfo* (*Pa_GetDeviceInfo)( PaDeviceIndex device );  //////////////////////////////////////////////////////////////////////////////////// 

typedef struct PaStreamParameters {
    PaDeviceIndex device;
    int channelCount;
    PaSampleFormat sampleFormat;
    PaTime suggestedLatency;
    void *hostApiSpecificStreamInfo;
} PaStreamParameters;

#define paFormatIsSupported (0)

PaError (*Pa_IsFormatSupported)( const PaStreamParameters *inputParameters,
                              const PaStreamParameters *outputParameters,
                              double sampleRate );   //////////////////////////////////////////////////////////////////////////////////// 

typedef void PaStream;

#define paFramesPerBufferUnspecified  (0)

typedef unsigned long PaStreamFlags;

#define   paNoFlag          ((PaStreamFlags) 0)
#define   paClipOff         ((PaStreamFlags) 0x00000001)
#define   paDitherOff       ((PaStreamFlags) 0x00000002)
#define   paNeverDropInput  ((PaStreamFlags) 0x00000004)
#define   paPrimeOutputBuffersUsingStreamCallback ((PaStreamFlags) 0x00000008)
#define   paPlatformSpecificFlags ((PaStreamFlags)0xFFFF0000)

typedef struct PaStreamCallbackTimeInfo{
    PaTime inputBufferAdcTime;
    PaTime currentTime;
    PaTime outputBufferDacTime;
} PaStreamCallbackTimeInfo;

typedef unsigned long PaStreamCallbackFlags;

#define paInputUnderflow   ((PaStreamCallbackFlags) 0x00000001)
#define paInputOverflow    ((PaStreamCallbackFlags) 0x00000002)
#define paOutputUnderflow  ((PaStreamCallbackFlags) 0x00000004)
#define paOutputOverflow   ((PaStreamCallbackFlags) 0x00000008)
#define paPrimingOutput    ((PaStreamCallbackFlags) 0x00000010)

typedef enum PaStreamCallbackResult {
    paContinue=0,
    paComplete=1,
    paAbort=2 
} PaStreamCallbackResult;

typedef int PaStreamCallback(
    const void *input, void *output,
    unsigned long frameCount,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData );

PaError (*Pa_OpenStream)( PaStream** stream,
                       const PaStreamParameters *inputParameters,
                       const PaStreamParameters *outputParameters,
                       double sampleRate,
                       unsigned long framesPerBuffer,
                       PaStreamFlags streamFlags,
                       PaStreamCallback *streamCallback,
                       void *userData );  //////////////////////////////////////////////////////////////////////////////////// 

PaError (*Pa_OpenDefaultStream)( PaStream** stream,
                              int numInputChannels,
                              int numOutputChannels,
                              PaSampleFormat sampleFormat,
                              double sampleRate,
                              unsigned long framesPerBuffer,
                              PaStreamCallback *streamCallback,
                              void *userData );   //////////////////////////////////////////////////////////////////////////////////// 

PaError (*Pa_CloseStream)( PaStream *stream );  //////////////////////////////////////////////////////////////////////////////////// 

typedef void PaStreamFinishedCallback( void *userData );

PaError (*Pa_SetStreamFinishedCallback)( PaStream *stream, PaStreamFinishedCallback* streamFinishedCallback );   //////////////////////////////////////////////////////////////////////////////////// 
PaError (*Pa_StartStream)( PaStream *stream );  //////////////////////////////////////////////////////////////////////////////////// 
PaError (*Pa_StopStream)( PaStream *stream );  //////////////////////////////////////////////////////////////////////////////////// 
PaError (*Pa_AbortStream)( PaStream *stream );  //////////////////////////////////////////////////////////////////////////////////// 
PaError (*Pa_IsStreamStopped)( PaStream *stream );  //////////////////////////////////////////////////////////////////////////////////// 
PaError (*Pa_IsStreamActive)( PaStream *stream );  //////////////////////////////////////////////////////////////////////////////////// 

typedef struct PaStreamInfo {
    int structVersion;
    PaTime inputLatency;
    PaTime outputLatency;
    double sampleRate;
} PaStreamInfo;

const PaStreamInfo* (*Pa_GetStreamInfo)( PaStream *stream );  //////////////////////////////////////////////////////////////////////////////////// 
PaTime              (*Pa_GetStreamTime)( PaStream *stream );  //////////////////////////////////////////////////////////////////////////////////// 
double              (*Pa_GetStreamCpuLoad)( PaStream* stream );  //////////////////////////////////////////////////////////////////////////////////// 
PaError             (*Pa_ReadStream)( PaStream* stream, void *buffer, unsigned long frames );  //////////////////////////////////////////////////////////////////////////////////// 
PaError             (*Pa_WriteStream)( PaStream* stream, const void *buffer, unsigned long frames );  //////////////////////////////////////////////////////////////////////////////////// 
signed long         (*Pa_GetStreamReadAvailable)( PaStream* stream );  //////////////////////////////////////////////////////////////////////////////////// 
signed long         (*Pa_GetStreamWriteAvailable)( PaStream* stream );  //////////////////////////////////////////////////////////////////////////////////// 
PaError             (*Pa_GetSampleSize)( PaSampleFormat format );  //////////////////////////////////////////////////////////////////////////////////// 
void                (*Pa_Sleep)( long msec );  //////////////////////////////////////////////////////////////////////////////////// 


// ##########################################################################################################################
// ##########################################################################################################################
// ##########################################################################################################################


void (*PaUtil_InitializeClock)( void );
double (*PaUtil_GetTime)( void );
PaError (*PaAsio_ShowControlPanel)( PaDeviceIndex , void* );


int portaduio_import(){
    HMODULE dll = LoadLibrary( "portaudio.dll" );
    if( !dll ) return 1;
    Pa_GetVersion = GetProcAddress( dll, "Pa_GetVersion" );
    if( !Pa_GetVersion ) return 2;
    Pa_GetVersionInfo = GetProcAddress( dll, "Pa_GetVersionInfo" );
    Pa_GetErrorText = GetProcAddress( dll, "Pa_GetErrorText" );
    Pa_Initialize = GetProcAddress( dll, "Pa_Initialize" );
    Pa_Terminate = GetProcAddress( dll, "Pa_Terminate" );
    Pa_GetHostApiCount = GetProcAddress( dll, "Pa_GetHostApiCount" );
    Pa_GetDefaultHostApi = GetProcAddress( dll, "Pa_GetDefaultHostApi" );
    Pa_GetHostApiInfo = GetProcAddress( dll, "Pa_GetHostApiInfo" );
    Pa_HostApiTypeIdToHostApiIndex = GetProcAddress( dll, "Pa_HostApiTypeIdToHostApiIndex" );
    Pa_HostApiDeviceIndexToDeviceIndex = GetProcAddress( dll, "Pa_HostApiDeviceIndexToDeviceIndex" );
    Pa_GetLastHostErrorInfo = GetProcAddress( dll, "Pa_GetLastHostErrorInfo" );
    Pa_GetDeviceCount = GetProcAddress( dll, "Pa_GetDeviceCount" );
    Pa_GetDefaultInputDevice = GetProcAddress( dll, "Pa_GetDefaultInputDevice" );
    Pa_GetDefaultOutputDevice = GetProcAddress( dll, "Pa_GetDefaultOutputDevice" );
    Pa_GetDeviceInfo = GetProcAddress( dll, "Pa_GetDeviceInfo" );
    Pa_IsFormatSupported = GetProcAddress( dll, "Pa_IsFormatSupported" );
    Pa_OpenStream = GetProcAddress( dll, "Pa_OpenStream" );
    Pa_OpenDefaultStream = GetProcAddress( dll, "Pa_OpenDefaultStream" );
    Pa_CloseStream = GetProcAddress( dll, "Pa_CloseStream" );
    Pa_SetStreamFinishedCallback = GetProcAddress( dll, "Pa_SetStreamFinishedCallback" );
    Pa_StartStream = GetProcAddress( dll, "Pa_StartStream" );
    Pa_StopStream = GetProcAddress( dll, "Pa_StopStream" );
    Pa_AbortStream = GetProcAddress( dll, "Pa_AbortStream" );
    Pa_IsStreamStopped = GetProcAddress( dll, "Pa_IsStreamStopped" );
    Pa_IsStreamActive = GetProcAddress( dll, "Pa_IsStreamActive" );
    Pa_GetStreamInfo = GetProcAddress( dll, "Pa_GetStreamInfo" );
    Pa_GetStreamTime = GetProcAddress( dll, "Pa_GetStreamTime" );
    Pa_GetStreamCpuLoad = GetProcAddress( dll, "Pa_GetStreamCpuLoad" );
    Pa_ReadStream = GetProcAddress( dll, "Pa_ReadStream" );
    Pa_WriteStream = GetProcAddress( dll, "Pa_WriteStream" );
    Pa_GetStreamReadAvailable = GetProcAddress( dll, "Pa_GetStreamReadAvailable" );
    Pa_GetStreamWriteAvailable = GetProcAddress( dll, "Pa_GetStreamWriteAvailable" );
    Pa_GetSampleSize = GetProcAddress( dll, "Pa_GetSampleSize" );
    Pa_Sleep = GetProcAddress( dll, "Pa_Sleep" );
    
    PaUtil_InitializeClock = GetProcAddress( dll, "PaUtil_InitializeClock" );
    PaUtil_GetTime = GetProcAddress( dll, "PaUtil_GetTime" );
    PaAsio_ShowControlPanel = GetProcAddress( dll, "PaAsio_ShowControlPanel" );
    
    if( !PaUtil_InitializeClock || !PaUtil_GetTime ) return 3;
    if( !PaAsio_ShowControlPanel ) return 4;
    
    return 0;
}