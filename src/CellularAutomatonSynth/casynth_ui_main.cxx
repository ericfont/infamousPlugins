#ifndef CASYNTH_H
#define CASYNTH_H

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/x.H>

#include "casynth_ui.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"

#define CASYNTHUI_URI "http://infamousplugins.sourceforge.net/plugs.html#casynth_ui"

static LV2UI_Handle init_casynthUI(const struct _LV2UI_Descriptor * descriptor,
		const char * plugin_uri,
		const char * bundle_path,
		LV2UI_Write_Function write_function,
		LV2UI_Controller controller,
		LV2UI_Widget * widget,
		const LV2_Feature * const * features) 
{
    if(strcmp(plugin_uri, CASYNTH_URI) != 0)
    {
        return 0;
    }

    CaSynthUI* self = new CaSynthUI();
    if(!self) return 0;
    LV2UI_Resize* resize = NULL;

    self->controller = controller;
    self->write_function = write_function;

    void* parentXwindow = 0;
    for (int i = 0; features[i]; ++i)
    {
        if (!strcmp(features[i]->URI, LV2_UI__parent)) 
        {
           parentXwindow = features[i]->data;
        }
	else if (!strcmp(features[i]->URI, LV2_UI__resize)) 
        {
           resize = (LV2UI_Resize*)features[i]->data;
        }
    }

    self->ui = self->show();
    srand ((unsigned int) time (NULL));
    fl_open_display();
    // set host to change size of the window
    if (resize)
    {
      //self->ui->size(400,200);//test different aspect ratios for when it will be resizable
      resize->ui_resize(resize->handle, self->ui->w(), self->ui->h());
    }
    fl_embed( self->ui,(Window)parentXwindow);

    return (LV2UI_Handle)self;
}

void cleanup_casynthUI(LV2UI_Handle ui)
{
    CaSynthUI *self = (CaSynthUI*)ui;

    delete self->ui;
    delete self;
}

void casynthUI_port_event(LV2UI_Handle ui, uint32_t port_index, uint32_t buffer_size, uint32_t format, const void * buffer)
{
    CaSynthUI *self = (CaSynthUI*)ui;
    if(!format)
    {
      float val = *(float*)buffer;
      char str[5];
      switch(port_index)
      {
        case CHANNEL:
          self->channel->value(val);
	  break;
        case MASTER_GAIN:
          self->mastergain->value(val);
	  break;
        case WAVE:
          self->cellwaveform->value(val);
	  break;
        case CELL_LIFE:
          self->lifetime->value(val);
	  break;
        case HARM_MODE:
          self->harmgain->value(val);
	  break;
        case NHARMONICS:
          self->nharmonics->value(val);
	  break;
        case HARM_WIDTH:
          self->harmwidth->value(val);
	  break;
        case ENV_A:
          self->a->value(val);
	  break;
        case ENV_D:
          self->d->value(val);
	  break;
        case ENV_B:
          self->b->value(val);
	  break;
        case ENV_SWL:
          self->sw->value(val);
	  break;
        case ENV_SUS:
          self->su->value(val);
	  break;
        case ENV_R:
          self->r->value(val);
	  break;
        case AMOD_WAV:
          self->amwave->value(val);
	  break;
        case AMOD_FREQ:
          self->amfreq->value(val);
	  break;
        case AMOD_GAIN:
          self->amgain->value(val);
	  break;
        case FMOD_WAV:
          self->fmwave->value(val);
	  break;
        case FMOD_FREQ:
          self->fmfreq->value(val);
	  break;
        case FMOD_GAIN:
          self->fmgain->value(val);
	  break;
        case RULE:
          self->set_rule(val);
	  break;
        case INIT_CELLS:
          self->set_initial_condition(val);
	  break;
      }//switch
    }//if float
}

static int
idle(LV2UI_Handle handle)
{
  CaSynthUI* self = (CaSynthUI*)handle;
  self->idle();
  
  return 0;
}

static int
resize_func(LV2UI_Feature_Handle handle, int w, int h)
{
  CaSynthUI* self = (CaSynthUI*)handle;
  self->ui->size(w,h);
  
  return 0;
}

static const LV2UI_Idle_Interface idle_iface = { idle };
static const LV2UI_Resize resize_ui = { 0, resize_func };//ideally 1st member would be the CaSynthUI instance

static const void*
extension_data(const char* uri)
{
  if (!strcmp(uri, LV2_UI__idleInterface))
  {
    return &idle_iface;
  }
  if (!strcmp(uri, LV2_UI__resize))
  {
    return &resize_ui;
  }
  return NULL;
}

static const LV2UI_Descriptor casynthUI_descriptor = {
    CASYNTHUI_URI,
    init_casynthUI,
    cleanup_casynthUI,
    casynthUI_port_event,
    extension_data
};

LV2_SYMBOL_EXPORT 
const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index) 
{
    switch (index) {
    case 0:
        return &casynthUI_descriptor;
    default:
        return NULL;
    }
}
#endif
