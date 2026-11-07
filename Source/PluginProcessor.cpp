#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

const std::array<const char*,9> LTASoundbankAudioProcessor::categoryNames = {{
    "Basses", "Leads", "Plucks", "Keys", "Synths & Pads", "Sequences & Arps", "Vox & Choir", "Strings", "Guitars"
}};

namespace
{
constexpr float tau = juce::MathConstants<float>::twoPi;
float fract(float x) { return x-std::floor(x); }
uint32_t mixHash(uint32_t v)
{
    v ^= v >> 16; v *= 0x7feb352du; v ^= v >> 15; v *= 0x846ca68bu; v ^= v >> 16;
    return v;
}
float rand01(uint32_t seed) { return (mixHash(seed)&0xffffffu)/16777215.0f; }
float oscillator(int type, float phase, float phase2, float fm)
{
    float p=fract(phase), q=fract(phase2);
    switch(type%10)
    {
        case 0: return std::sin(tau*(p + fm*0.1f*std::sin(tau*q)));
        case 1: return 2.0f*p-1.0f;
        case 2: return 1.0f-4.0f*std::abs(p-0.5f);
        case 3: return p<0.45f ? 1.0f:-1.0f;
        case 4: return 0.7f*std::sin(tau*p)+0.3f*std::sin(3*tau*q);
        case 5: return std::sin(tau*(p + fm*0.35f*std::sin(tau*q)));
        case 6: return 0.52f*(2*p-1)+0.48f*std::sin(tau*q);
        case 7: return 0.65f*std::sin(tau*p)+0.35f*std::sin(5*tau*q);
        case 8: return std::tanh(2.5f*std::sin(tau*p));
        default: return 0.55f*std::sin(tau*p)+0.3f*std::sin(2*tau*q)+0.15f*std::sin(4*tau*p);
    }
}
std::array<LTAPreset,400> makePresets()
{
    std::array<LTAPreset,400> all;
    const std::array<const char*,20> adjectives {{
        "Velvet","Chrome","Cloud","Nocturne","Static","Hollow","Lucid","Iridescent","Halo","Dusk",
        "Ghost","Satin","Neon","Polar","Mirage","Silver","Fever","Distant","Glass","Afterglow" }};
    const std::array<const char*,25> nouns {{
        "Drift","Bloom","Signal","Orbit","Ritual","Haze","Fall","Echo","Ray","Pulse",
        "Dream","Current","Motion","Rain","Reflection","Mist","Circuit","Garden","Tide","Cascade",
        "Memory","Glow","Sequence","Shadow","Dust" }};
    for(int c=0;c<9;++c)
    for(int p=0;p<50;++p)
    {
        const int id=c*50+p;
        auto& x=all[(size_t)id];
        x.category=c;
        x.name=juce::String(adjectives[(size_t)((p*7+c*3)%20)]) + " " + nouns[(size_t)((p*11+(p/20)+c*5)%25)];
        // Unique numeric fingerprints and 10 oscillator algorithms per category.
        const uint32_t seed=(uint32_t)(id+1)*7919u;
        x.type=(p+c*3)%10;
        x.attack=0.004f+rand01(seed+11)*0.12f;
        x.decay=0.08f+rand01(seed+12)*1.15f;
        x.sustain=0.2f+rand01(seed+13)*0.65f;
        x.release=0.09f+rand01(seed+14)*1.4f;
        x.brightness=0.25f+rand01(seed+15)*0.7f;
        x.resonance=rand01(seed+16)*0.35f;
        x.detune=rand01(seed+17)*0.022f;
        x.fm=rand01(seed+18)*0.75f;
        x.noise=rand01(seed+19)*0.24f;
        x.vibrato=rand01(seed+20)*0.010f;
        x.drive=rand01(seed+21)*0.55f;
        x.space=rand01(seed+22)*0.38f;
        x.motion=rand01(seed+23)*0.5f;
        x.harmonic=rand01(seed+24);
        x.level=0.16f+rand01(seed+25)*0.13f;
        x.arpRate=2+(p%4);
        x.arpStyle=p%5;
        if(c==0) // Bass & 808
        {
            x.attack=0.002f; x.decay=0.2f+rand01(seed+30)*0.8f;
            x.sustain=0.1f+rand01(seed+31)*0.68f;
            x.release=0.05f+rand01(seed+32)*0.18f;
            x.brightness=0.09f+rand01(seed+33)*0.30f;
            x.drive=0.2f+rand01(seed+34)*0.74f;
            x.space=0; x.noise=rand01(seed+35)*0.05f;
            x.type=(p%4==0)?0:(p%4==1?3:(p%4==2?5:6));
        }
        else if(c==1) // Leads
        {
            x.attack=0.004f+rand01(seed+35)*0.04f;
            x.sustain=0.6f+rand01(seed+36)*0.35f;
            x.release=0.09f+rand01(seed+37)*0.35f;
            x.brightness=0.45f+rand01(seed+38)*0.55f;
        }
        else if(c==2) // Plucks
        {
            x.attack=0.001f+rand01(seed+38)*0.009f;
            x.decay=0.08f+rand01(seed+39)*0.62f;
            x.sustain=0.02f+rand01(seed+40)*0.10f;
            x.release=0.08f+rand01(seed+41)*0.65f;
            x.type=(p*3+2)%10;
        }
        else if(c==3) // Keys
        {
            x.attack=0.002f+rand01(seed+42)*0.024f;
            x.decay=0.5f+rand01(seed+43)*1.7f;
            x.sustain=0.10f+rand01(seed+44)*0.30f;
            x.release=0.22f+rand01(seed+45)*0.7f;
            x.type=(p%2)?4:7;
        }
        else if(c==4) // Pads
        {
            x.attack=0.24f+rand01(seed+46)*1.35f;
            x.release=0.9f+rand01(seed+47)*2.25f;
            x.sustain=0.55f+rand01(seed+48)*0.4f;
            x.space=0.22f+rand01(seed+49)*0.5f;
        }
        else if(c==5) // Sequences
        {
            x.attack=0.003f; x.decay=0.09f+rand01(seed+50)*0.35f;
            x.sustain=0.1f; x.release=0.06f+rand01(seed+51)*0.14f;
        }
        else if(c==6) // Synthetic vowel choir
        {
            x.attack=0.14f+rand01(seed+52)*0.55f;
            x.release=0.5f+rand01(seed+53)*1.4f;
            x.type=9; x.fm=0.0f; x.space=0.35f;
        }
        else if(c==7) // Synth strings
        {
            x.attack=0.045f+rand01(seed+54)*0.32f;
            x.release=0.28f+rand01(seed+55)*1.25f;
            x.type=(p%2)?1:6;
            x.noise=0.04f+rand01(seed+56)*0.12f;
        }
        else // Guitars, 25 electric and 25 acoustic-like synthesized plucks
        {
            x.attack=0.001f; x.decay=0.27f+rand01(seed+57)*1.2f;
            x.sustain=0.04f+rand01(seed+58)*0.18f;
            x.release=0.13f+rand01(seed+59)*0.75f;
            x.type=p<25?((p%2)?6:8):((p%2)?4:7);
            x.drive=p<25?0.22f+rand01(seed+60)*0.62f:rand01(seed+61)*0.10f;
            x.noise=0.02f+rand01(seed+62)*0.12f;
        }
    }
    return all;
}
}

const std::array<LTAPreset,400>& LTASoundbankAudioProcessor::presets()
{
    static const auto data=makePresets();
    return data;
}

juce::AudioProcessorValueTreeState::ParameterLayout LTASoundbankAudioProcessor::makeParams()
{
    juce::AudioProcessorValueTreeState::ParameterLayout result;
    result.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"preset",1},"Preset",0,399,0));
    return result;
}

LTASoundbankAudioProcessor::LTASoundbankAudioProcessor()
: juce::AudioProcessor(juce::AudioProcessor::BusesProperties()
    .withOutput("Output",juce::AudioChannelSet::stereo(),true)),
  parameters(*this,nullptr,"LTAState",makeParams())
{
    for(auto& f:favourites) f.store(false);
    for(auto& s:scope) s.store(0.0f);
}

void LTASoundbankAudioProcessor::prepareToPlay(double sampleRate,int)
{
    sr=std::max(1000.0,sampleRate); sampleClock=0; arpClock=0; arpStep=0;
    previousArpNote=-1; previewRemaining=0;
    for(auto& v:voices) v=Voice{};
    std::fill(std::begin(delayL),std::end(delayL),0.0f);
    std::fill(std::begin(delayR),std::end(delayR),0.0f);
    delayPtr=0;
}

bool LTASoundbankAudioProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    return layout.getMainOutputChannelSet()==juce::AudioChannelSet::mono()
        || layout.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();
}

int LTASoundbankAudioProcessor::getCurrentProgram() {return selectedPreset();}
void LTASoundbankAudioProcessor::setCurrentProgram(int index) {selectPreset(index);}
const juce::String LTASoundbankAudioProcessor::getProgramName(int index)
{
    return presets()[(size_t)juce::jlimit(0,399,index)].name;
}
void LTASoundbankAudioProcessor::selectPreset(int index)
{
    index=juce::jlimit(0,399,index);
    if(auto* p=parameters.getParameter("preset"))
        p->setValueNotifyingHost(p->convertTo0to1((float)index));
    presetIndex.store(index);
}
void LTASoundbankAudioProcessor::setFavourite(int idx,bool value)
{
    if(idx>=0 && idx<numPresets) favourites[(size_t)idx].store(value);
}
bool LTASoundbankAudioProcessor::isFavourite(int idx) const
{
    return idx>=0 && idx<numPresets && favourites[(size_t)idx].load();
}
float LTASoundbankAudioProcessor::scopeSample(int index) const
{
    return scope[(size_t)((index%512+512)%512)].load(std::memory_order_relaxed);
}
void LTASoundbankAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    juce::XmlElement root("LTA_SOUNDBANK");
    root.setAttribute("preset",selectedPreset());
    juce::String f;
    for(int i=0;i<400;i++) if(isFavourite(i)) f+=juce::String(i)+",";
    root.setAttribute("favourites",f);
   copyXmlToBinary(root,dest);
}
void LTASoundbankAudioProcessor::setStateInformation(const void* data,int size)
{
    if(auto root=getXmlFromBinary(data,size))
    {
        if(root->hasTagName("LTA_SOUNDBANK"))
        {
            selectPreset(root->getIntAttribute("preset",0));
            for(auto& x:favourites) x.store(false);
            juce::StringArray ids;
            ids.addTokens(root->getStringAttribute("favourites"),",","");
            for(auto& id:ids) setFavourite(id.getIntValue(),true);
        }
    }
}

int LTASoundbankAudioProcessor::voiceFor(int note)
{
    for(size_t i=0;i<voices.size();++i)
        if(voices[i].active && voices[i].note==note) return (int)i;
    for(size_t i=0;i<voices.size();++i)
        if(!voices[i].active) return (int)i;
    auto oldest=std::max_element(voices.begin(),voices.end(),
        [](const Voice&a,const Voice&b){return a.age<b.age;});
    return (int)std::distance(voices.begin(),oldest);
}
void LTASoundbankAudioProcessor::noteOn(int note,float velocity,const LTAPreset&)
{
    if(note<0 || note>127) return;
    auto& v=voices[(size_t)voiceFor(note)];
    v=Voice{}; v.active=true; v.held=true; v.note=note;
    v.velocity=juce::jlimit(0.0f,1.0f,velocity);
    v.lastHz=440.0f*std::pow(2.0f,(note-69)/12.0f);
    v.stage=0;
}
void LTASoundbankAudioProcessor::noteOff(int note)
{
    for(auto& v:voices)
        if(v.active && v.note==note) {v.held=false; if(v.stage<3) v.stage=3;}
}
void LTASoundbankAudioProcessor::handleMidi(const juce::MidiMessage& msg,const LTAPreset& p)
{
    if(msg.isAllNotesOff() || msg.isAllSoundOff())
    {
        heldNotes.fill(false);
        for(auto& v:voices) v.active=false;
        return;
    }
    if(msg.isNoteOn())
    {
        const int n=msg.getNoteNumber(); heldNotes[(size_t)n]=true;
        if(p.category!=5) noteOn(n,msg.getFloatVelocity(),p);
    }
    else if(msg.isNoteOff())
    {
        const int n=msg.getNoteNumber(); heldNotes[(size_t)n]=false;
        if(p.category!=5) noteOff(n);
    }
}
float LTASoundbankAudioProcessor::renderVoice(Voice& v,const LTAPreset& p)
{
    const float dt=1.0f/(float)sr;
    const float att=std::max(0.001f,p.attack);
    const float dec=std::max(0.005f,p.decay);
    const float rel=std::max(0.01f,p.release);
    switch(v.stage)
    {
        case 0: v.env+=dt/att; if(v.env>=1.0f){v.env=1;v.stage=1;} break;
        case 1: v.env+=(p.sustain-v.env)*juce::jmin(1.0f,dt*5.0f/dec);
                if(v.age>(int)((att+dec)*sr)) v.stage=2; break;
        case 2: if(!v.held) v.stage=3; break;
        default: v.env-=dt/rel; if(v.env<=0) {v.active=false;return 0;} break;
    }
    const float hz=440.0f*std::pow(2.0f,(v.note-69)/12.0f);
    const float vibrato=1.0f+p.vibrato*std::sin(tau*(float)(v.age*dt)*(3.2f+p.motion*9.0f));
    const float freq=std::min((float)sr*0.45f,hz*vibrato);
    v.phase=fract(v.phase+freq*dt);
    v.phase2=fract(v.phase2+freq*dt*(1.0f+p.detune*2.0f)*(1.0f+p.harmonic*0.13f));
    const uint32_t n=mixHash((uint32_t)(v.age+1)*1973u+(uint32_t)(v.note*3457));
    const float noise=((n&65535u)/32767.5f-1.0f);
    float sig=oscillator(p.type,v.phase,v.phase2,p.fm);
    sig+=(p.detune*13.0f)*oscillator((p.type+2)%10,v.phase2,v.phase,p.fm*0.5f);
    sig+=noise*p.noise;
    if(p.category==6)
    {
        // Artificial vowel resonances, deliberately synthetic rather than sampled vocals.
        const float vowels=std::sin(tau*v.phase*2.0f)*0.33f + std::sin(tau*v.phase2*3.0f)*0.19f;
        sig=0.55f*sig+vowels;
    }
    if(p.category==0 && v.age<(int)(sr*0.055))
    {
        const float transient=1.0f-(float)v.age/(float)(sr*0.055);
        sig+=transient*0.16f*std::sin(tau*v.phase*2.0f);
    }
    const float cutoff=juce::jlimit(45.0f,(float)sr*0.35f,
        75.0f+std::pow(p.brightness,2.0f)*12000.0f*(0.7f+0.3f*v.env));
    const float alpha=1.0f-std::exp(-tau*cutoff*dt);
    v.filter+=alpha*(sig-v.filter);
    v.lowpass2+=alpha*(v.filter-v.lowpass2);
    float output=0.6f*v.filter+0.4f*v.lowpass2;
    const float drive=1.0f+p.drive*9.0f;
    output=std::tanh(output*drive)/(std::tanh(drive)+0.001f);
    ++v.age;
    return output*v.env*v.velocity*p.level;
}
void LTASoundbankAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    const int n=buffer.getNumSamples();
    if(n<=0 || buffer.getNumChannels()<=0) return;
    const int current=juce::jlimit(0,399,(int)std::round(parameters.getRawParameterValue("preset")->load()));
    presetIndex.store(current);
    const auto& p=presets()[(size_t)current];
    if(previewRequest.exchange(false))
    {
        noteOn(p.category==0?36:60,0.8f,p);
        previewRemaining=(int)(sr*0.32);
    }
    auto playhead=getPlayHead();
    if(playhead!=nullptr)
    {
        if(auto info=playhead->getPosition())
            if(auto bpm=info->getBpm()) lastBpm=juce::jlimit(30.0,300.0,*bpm);
    }
    auto it=midi.cbegin();
    auto end=midi.cend();
    int scopeCursor=scopeHead.load(std::memory_order_relaxed);
    float maxAmp=0;
    for(int s=0;s<n;++s)
    {
        while(it!=end && (*it).samplePosition<=s)
        {
            handleMidi((*it).getMessage(),p);
            ++it;
        }
        if(previewRemaining>0 && --previewRemaining==0) noteOff(p.category==0?36:60);
        if(p.category==5)
        {
            // Host tempo synced step sequencer, one note/chord input triggers a sequence.
            const double samplesPerStep=sr*60.0/lastBpm/(double)p.arpRate;
            arpClock+=1.0;
            if(arpClock>=samplesPerStep)
            {
                arpClock-=samplesPerStep;
                if(previousArpNote>=0) noteOff(previousArpNote);
                int root=-1;
                for(int j=0;j<128;++j) if(heldNotes[(size_t)j]) {root=j;break;}
                if(root>=0)
                {
                    static const int patterns[5][8]={{0,4,7,12,7,4,0,7},{0,7,12,7,5,7,12,4},
                        {0,3,7,10,7,3,12,7},{0,12,7,5,12,7,5,3},{0,5,7,3,10,7,12,5}};
                    previousArpNote=juce::jlimit(0,127,root+patterns[p.arpStyle][arpStep%8]);
                    noteOn(previousArpNote,0.7f,p);
                    ++arpStep;
                }
                else previousArpNote=-1;
            }
        }
        float out=0;
        for(auto& v:voices) if(v.active) out+=renderVoice(v,p);
        // A gentle stereo diffusion based on delay feedback; no allocations in audio thread.
        const int d1=juce::jlimit(1,32000,(int)(sr*(0.10f+p.space*0.21f)));
        const int d2=juce::jlimit(1,32000,(int)(sr*(0.14f+p.space*0.29f)));
        const int i1=(delayPtr-d1+32768)%32768;
        const int i2=(delayPtr-d2+32768)%32768;
        const float tapL=delayL[i1], tapR=delayR[i2];
        delayL[delayPtr]=out+tapR*p.space*0.27f;
        delayR[delayPtr]=out+tapL*p.space*0.24f;
        delayPtr=(delayPtr+1)%32768;
        const float l=std::tanh(out+tapL*p.space*0.53f);
        const float r=std::tanh(out+tapR*p.space*0.53f);
        buffer.setSample(0,s,l);
        if(buffer.getNumChannels()>1) buffer.setSample(1,s,r);
        maxAmp=std::max(maxAmp,std::max(std::abs(l),std::abs(r)));
        if(s%8==0)
        {
            scope[(size_t)scopeCursor].store(0.5f*(l+r),std::memory_order_relaxed);
            scopeCursor=(scopeCursor+1)%512;
        }
        ++sampleClock;
    }
    scopeHead.store(scopeCursor,std::memory_order_release);
    levelMeter.store(maxAmp,std::memory_order_relaxed);
}
juce::AudioProcessorEditor* LTASoundbankAudioProcessor::createEditor()
{
    return new LTASoundbankAudioProcessorEditor(*this);
}
