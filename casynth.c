//Spencer Jackson
//casynth.c
#include<casynth.h>
#include<constants.h>
#include<string.h>

//private functions
void run_active_notes(CASYNTH *synth, uint32_t nframes, float buffer[]);

LV2_Handle init_casynth(const LV2_Descriptor *descriptor,double sample_rate, const char *bundle_path,const LV2_Feature * const* host_features)
{
    CASYNTH* synth = malloc(sizeof(CASYNTH));
    unsigned char i;

    synth->sample_rate = sample_rate;

    synth->midi_in_p = NULL;
    synth->nactive = 0;
    synth->nsustained = 0;
    for(i=0;i<127;i++)
    {
        init_note(&(note[i]),sample_rate,i,synth->nharmonics_p,synth->amod_gain_p,synth->fmod_gain_p);
        synth->active[i] = 0;
        synth->sustained[i] = 0;
    }

    synth->harmonic_mode = HARMONIC_MODE_SINC;
    synth->harm_gain_sinc[i] = 1/(MAX_N_HARMONICS +1);//(nharmonics+1);
    synth->harm_gain_saw[i] = .29/(i+2);//.29 makes it so gain=1 if all harmonics play
    synth->harm_gain_sqr[i] = (i%2!=0)*.48/(i+2);//odd harmonics
    synth->harm_gain_tri[i] = (i%2!=0)*.83/((i+2)*(i+2));
    synth->harm_gains = synth->harm_gain_sinc;

    //get urid map value for midi events
    for (int i = 0; host_features[i]; ++i)
    {
        if (strcmp(host_features[i]->URI, LV2_URID_MAP_URI) == 0)
        {
            LV2_URID_Map *urid_map = (LV2_URID_Map *) host_features[i]->data;
            if (urid_map)
            {
                synth->midi_event_type = urid_map->map(urid_map->handle, LV2_MIDI__MidiEvent);
                break;
            }
        }
    }

    return synth;
}

static void connect_casynth_ports(LV2_Handle handle, uint32_t port, void *data)
{
    CASYNTH* synth = (CASYNTH*)handle;
    if(port == MIDI_IN)         synth->midi_in_p = (LV2_Atom_Sequence*)data;
    else if(port == OUTPUT)     synth->output_p = (float*)data;
    else if(port == CHANNEL)    synth->channel_p = (float*)data;
    else if(port == MASTER_GAIN)synth->master_gain_p = (float*)data;
    else if(port == RULE)       synth->cell_life_p = (float*)data;
    else if(port == CELL_LIFE)  synth->rule_p = (float*)data;
    else if(port == INIT_CELLS) synth->init_cells_p = (float*)data;
    else if(port == NHARMONICS) synth->nharmonics_p = (float*)data;
    else if(port == HARM_MODE)  synth->harmonic_mode_p = (float*)data;
    else if(port == WAVE)       synth->wave_p = (float*)data;
    else if(port == ENV_A)      synth->env_a_p = (float*)data;
    else if(port == ENV_D)      synth->env_d_p = (float*)data;
    else if(port == ENV_B)      synth->env_b_p = (float*)data;
    else if(port == ENV_SWL)    synth->env_swl_p = (float*)data;
    else if(port == ENV_SUS)    synth->env_sus_p = (float*)data;
    else if(port == ENV_R)      synth->env_r_p = (float*)data;
    else if(port == AMOD_WAV)   synth->amod_wave_p = (float*)data;
    else if(port == AMOD_FREQ)  synth->amod_freq_p = (float*)data;
    else if(port == AMOD_GAIN)  synth->amod_gain_p = (float*)data;
    else if(port == FMOD_WAV)   synth->fmod_wave_p = (float*)data;
    else if(port == FMOD_FREQ)  synth->fmod_freq_p = (float*)data;
    else if(port == FMOD_GAIN)  synth->fmod_gain_p = (float*)data;
    else puts("UNKNOWN PORT YO!!");
}

static void run_casynth( LV2_Handle handle, uint32_t nframes)
{
    CASYNTH* synth = (CASYNTH*)handle;
    unsigned char i;
    float* buf = synth->output_p;
    LV2_Atom_Event event;
    uint32_t frame_no = 0;
    unsigned char* message;
    unsigned char type;
    unsigned char num, val;
    short bend;

    for(i=0;i<nframes;i++)//start by filling buffer with 0s, we'll add to this
        buf[i] = 0;

    if (synth->midi_in_p)
    {
        LV2_ATOM_SEQUENCE_FOREACH(synth->midi_in_p, event)
        {
            if (event && event->body.type == synth->midi_event_type)//make sure its a midi event
            {
                message = (unsigned char*) LV2_ATOM_BODY(&event->body);
                if(!(*synth->channel_p) || message[0]&MIDI_CHANNEL_MASK == *synth->channel_p+1)
                {
                    type = message[0]&MIDI_TYPE_MASK;

                    if(type == MIDI_NOTE_ON)
                    {
                        num = message[1]&MIDI_DATA_MASK;
                        val = message[2]&MIDI_DATA_MASK;
                        if(synth->note[num].note_dead == true)
                        {
                            synth->active[synth->nactive++] = num;//push new note onto active stack
                        }
                        start_note(&(synth->note[num]),
                                   val,
                                   event->time.frames,
                                   synth->harm_gains,
                                   *synth->init_cells_p,
                                   synth->envelope,
                                   *synth->wave_p,
                                   *synth->amod_wave_p,
                                   *synth->fmod_wave_p);
                    }
                    else if(type == MIDI_NOTE_OFF)
                    {
                        num = message[1]&MIDI_DATA_MASK;
                        for(i=0;i<synth->nactive;i++)
                        {
                            if(synth->active[i] == num)
                            {
                                if(synth->sus)
                                {
                                    if(!synth->note[num].sus)
                                    {
                                        synth->note[num].sus = true;
                                        synth->sustained[synth->nsustained++] = num;
                                    }
                                }
                                else
                                {
                                    end_note(&(synth->note[num]),event->time.frames);
                                }
                            }
                        }

                    }
                    else if(type == MIDI_CONTROL_CHANGE)
                    {
                        num = message[1]&MIDI_DATA_MASK;
                        if(num == MIDI_SUSTAIN_PEDAL)
                        {
                            val = message[2]&MIDI_DATA_MASK;
                            if(val < 64)
                            {
                                synth->sus = false;
                                //end all sus. notes
                                for(i=0;i<synth->nsustained;i++)
                                {
                                    end_note(&(synth->note[synth->sustained[i]]),event->time.frames);
                                }
                                synth->nsustained = 0;
                            }
                            else
                            {
                                synth->sus = true;
                            }
                        }

                    }
                    else if(type == MIDI_PITCHBEND)
                    {
                        bend = message[1]&MIDI_DATA_MASK + (message[2]&MIDI_DATA_MASK)<<7 - MIDI_PITCH_CENTER;
                        synth->pitchbend = pow(2,bend/49152);//49152 is 12*8192/2
                        //run and update current position because this blocks
                        run_active_notes(synth, event->time.frames - frame_no, &(buf[frame_no]));
                        frame_no = event->time.frames;
                    }//message types
                }//correct channel
            }//actually midi
        }//for each event
    }//there are events

    //finish off whatever frames are left
    if(frame_no != nframes-1)
        run_active_notes(synth, nframes - frame_no, &(buf[frame_no]));
}

void run_active_notes(CASYNTH *synth, uint32_t nframes, float buffer[])
{
    unsigned char i,j;
    NOTE* note;
    double astep = *synth->amod_freq_p/synth->sample_rate;
    double fstep = *synth->fmod_freq_p/synth->sample_rate;//need to decide where to calculate this. Probably not here.
    for(i=0;i<synth->nactive;i++)
    {
        note = &(synth->note[synth->active[i]]);
        play_note( note,
                   nframes,
                   buffer,
                   synth->pitchbend,
                   (unsigned char)*synth->rule_p,
                   astep,
                   fstep);
        //cleanup dead notes
        if(note->note_dead)
        {
            nactive--;
            for(j=i;j<nactive;j++)
            {
                synth->active[j] = synth->active[j+1];
            }
        }
    }
}
