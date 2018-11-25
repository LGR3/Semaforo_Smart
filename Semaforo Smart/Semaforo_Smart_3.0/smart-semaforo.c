#include <stdio.h>
#include <stdlib.h>
#include <portaudio.h>
#include <sndfile.h>
#include "audio.h"
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <unistd.h>
#include <sys/socket.h>

//busca dispositivos bluetooth proximos
//necessario beacon na botoeira do sinal
//localização pode ser feita pela aplicação buscando 
//o sinal bluetooth mais proximo do celular utilizado
int scan(int argc, char **argv)
{
    inquiry_info *ii = NULL;
    int max_rsp, num_rsp;
    int dev_id, sock, len, flags;
    int i;
    char addr[19] = { 0 };
    char name[248] = { 0 };

    dev_id = hci_get_route(NULL);
    sock = hci_open_dev( dev_id );
    if (dev_id < 0 || sock < 0) {
        perror("opening socket");
        exit(1);
    }

    len  = 8;
    max_rsp = 255;
    flags = IREQ_CACHE_FLUSH;
    ii = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));

    num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &ii, flags);
    if( num_rsp < 0 ) perror("hci_inquiry");

    for (i = 0; i < num_rsp; i++) {
        ba2str(&(ii+i)->bdaddr, addr);
        memset(name, 0, sizeof(name));
        if (hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(name), 
            name, 0) < 0)
        strcpy(name, "[unknown]");
        printf("%s  %s\n", addr, name);
    }

    free( ii );
    close( sock );
    return 0;
}

//Input vindo do botão (quanto tempo foi pressionado)
//Identifica se o pedestre precisa de tempo extendido
int pressBtn1 = 0;
int pressBtn2 = 0;
if(pressBtn1 > 3 || pressBtn2 > 3){
    if(pressBtn > 5 || pressBtn2 > 5){
        recordFLAC(AudioData data, const char *fileNam); //capta audio 
    }
    // Realizar regra para semaforo :
    // É necessario acesso ao software/hardware do semaforo para realizar as regras
    // as seguintes funções seriam utilizadas: acender luz, aumentar o tempo que
    // o sinal ficaria vermelho.
    semaforo.increaseTimer(10);
    semaforo.piscaLuz();
    semaforo.voiceCommad("Ajude de travessia");
}

// Verifica se ambos lados foram pressionados
if (pressBtn1 && pressBtn2){
    //indica prioridade para o semaforo
    //reduz tempo de espera
    //semaforo.timer -= 2
    semaforo.decreaseTimer(2);
}

AudioData initAudioData(uint32_t sampleRate, uint16_t channels, uint32_t duration)
{
    AudioData data;
    data.duration = duration;
    data.formatType = 1;
    data.numberOfChannels = channels;
    data.sampleRate = sampleRate;
    data.frameIndex = 0;
    data.maxFrameIndex = sampleRate * duration;
    return data;
}

static int recordCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
    AudioData* data = (AudioData*) userData;
    const float* rptr = (const float*)inputBuffer;
    float* wptr = &data->recordedSamples[data->frameIndex * data->numberOfChannels];
    long framesToCalc;
    int finished;
    unsigned long framesLeft = data->maxFrameIndex - data->frameIndex;

    (void) outputBuffer; /* Prevent unused variable warnings. */
    (void) timeInfo;
    (void) statusFlags;
    (void) userData;

    if(framesLeft < framesPerBuffer)
    {
        framesToCalc = framesLeft;
        finished = paComplete;
    }
    else
    {
        framesToCalc = framesPerBuffer;
        finished = paContinue;
    }

    if(!inputBuffer)
    {
        for(long i = 0; i < framesToCalc; i++)
        {
            *wptr++ = 0.0f;  // left
            if(data->numberOfChannels == 2) *wptr++ = 0.0f;  // right
        }
    }
    else
    {
        for(long i = 0; i < framesToCalc; i++)
        {
            *wptr++ = *rptr++;  // left
            if(data->numberOfChannels == 2) *wptr++ = *rptr++;  // right
        }
    }
    data->frameIndex += framesToCalc;
    return finished;
}


int recordFLAC(AudioData data, const char *fileName)
{
    PaStreamParameters inputParameters;
    PaStream* stream;
    int err = 0;
    int numSamples = data.maxFrameIndex * data.numberOfChannels;
    int numBytes = numSamples * sizeof(data.recordedSamples[0]);

    data.recordedSamples = calloc(numSamples, numBytes); // From now on, recordedSamples is initialised.
    if(!data.recordedSamples)
    {
        fprintf(stderr, "Could not allocate record array.\n");
        goto done;
    }
    if((err = Pa_Initialize())) goto done;

    inputParameters.device = Pa_GetDefaultInputDevice();
    if (inputParameters.device == paNoDevice)
    {
        fprintf(stderr,"Error: No default input device.\n");
        goto done;
    }
    inputParameters.channelCount = data.numberOfChannels;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(&stream, &inputParameters, NULL, data.sampleRate, paFramesPerBufferUnspecified, paClipOff, recordCallback, &data);
    if(err) goto done;

    if((err = Pa_StartStream(stream))) goto done;
    puts("=== Now recording!! Please speak into the microphone. ===");

    while(1 == (err = Pa_IsStreamActive(stream)))
    {
        Pa_Sleep(1000);
        printf("index = %d\n", data.frameIndex);
    }
    if((err = Pa_CloseStream(stream))) goto done;

done:
    Pa_Terminate();
    if(err)
    {
        fprintf(stderr, "An error occured while using the portaudio stream: %d\n", err);
        fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
        err = 1;      // Always manually return positive codes.
    }
    else
    {
        SF_INFO sfinfo;
        sfinfo.channels = data.numberOfChannels;
        sfinfo.samplerate = data.sampleRate;
        sfinfo.format = SF_FORMAT_FLAC | SF_FORMAT_PCM_16;
        SNDFILE* outfile = sf_open(fileName, SFM_WRITE, &sfinfo); // open to file
        if (!outfile) return -1;
        long wr = sf_write_float(outfile, data.recordedSamples, numSamples); // write the entire buffer to the file
        if (wr < numSamples) fprintf(stderr, "Only wrote %lu frames, should have written %d", wr, numSamples);
        sf_write_sync(outfile); // force write to disk
        sf_close(outfile); // don't forget to close the file
    }
    free(data.recordedSamples);
    return err;
}