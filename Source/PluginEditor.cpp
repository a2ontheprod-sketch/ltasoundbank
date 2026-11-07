#include "PluginEditor.h"
#include <cmath>

namespace
{
const juce::Colour bg(0xff181622);
const juce::Colour panel(0xff252031);
const juce::Colour panel2(0xff32283d);
const juce::Colour ink(0xfff7e8fa);
const juce::Colour muted(0xffab9cb8);
const juce::Colour accent(0xfff1b5e5);
const juce::Colour lilac(0xffae9bee);
constexpr float pi=juce::MathConstants<float>::pi;
}

LTASoundbankAudioProcessorEditor::LTASoundbankAudioProcessorEditor(LTASoundbankAudioProcessor& p)
: juce::AudioProcessorEditor(&p),processor(p)
{
    setSize(970,600);
    setResizable(true,true);
    setResizeLimits(800,540,1250,780);
    search.setTextToShowWhenEmpty("Search presets or moods...",juce::Colours::grey);
    search.setColour(juce::TextEditor::backgroundColourId,panel2);
    search.setColour(juce::TextEditor::textColourId,ink);
    search.setColour(juce::TextEditor::outlineColourId,juce::Colours::transparentBlack);
    search.setColour(juce::TextEditor::focusedOutlineColourId,lilac);
    search.setFont(juce::Font(juce::FontOptions(15.0f)));
    search.setBorder({5,13,5,8});
    search.onTextChange=[this]{firstVisible=0;rebuildList();repaint();};
    addAndMakeVisible(search);
    search.setVisible(false);
    rebuildList();
    lastPreset=processor.selectedPreset();
    startTimerHz(40);
}

void LTASoundbankAudioProcessorEditor::resized()
{
    const auto r=getLocalBounds();
    const int w=r.getWidth(),h=r.getHeight();
    backRect={w/2-145,h-119,48,43};
    nextRect={w/2+97,h-119,48,43};
    previewRect={w-188,84,132,38};
    surpriseRect={w-202,h-65,164,39};
    browseRect={30,h-65,148,39};
    favRect={w/2+186,h-119,48,43};
    waveRect={62,146,w-124,h-315};
    browserRect={22,78,w-44,h-154};
    searchRect={42,91,w-84,41};
    categoryRect={40,143,w-80,46};
    gridRect={40,203,w-80,h-296};
    search.setBounds(searchRect);
}

void LTASoundbankAudioProcessorEditor::drawPill(juce::Graphics& g,juce::Rectangle<int> r,juce::String text,bool active)
{
    g.setColour(active?juce::Colour(0xff74537f):panel2);
    g.fillRoundedRectangle(r.toFloat(),13.0f);
    g.setColour(active?ink:muted);
    g.setFont(juce::Font(juce::FontOptions(14.0f).withStyle("Bold")));
    g.drawFittedText(text,r.reduced(7,4),juce::Justification::centred,1);
}

void LTASoundbankAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bg);
    const int w=getWidth(),h=getHeight();
    g.setColour(accent);
    g.setFont(juce::Font(juce::FontOptions(24.0f).withStyle("Bold")));
    g.drawText("LTA SOUNDBANK",32,24,325,32,juce::Justification::centredLeft);
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText("400 SOUNDS   /   VOL. 1",33,55,280,18,juce::Justification::centredLeft);
    g.drawText("LTA LABS",w-151,34,116,22,juce::Justification::centredRight);
    if(browsing) drawBrowser(g); else drawPlay(g);
    g.setColour(juce::Colour(0xff201b2a));
    g.fillRoundedRectangle(juce::Rectangle<float>(19.0f,(float)h-70,(float)w-38,56.0f),16.0f);
    drawPill(g,browseRect,browsing?"PLAY":"BROWSE",browsing);
    drawPill(g,surpriseRect,"SURPRISE ME");
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawFittedText("Play MIDI notes in FL Studio  |  No editing needed",
        juce::Rectangle<int>(190,h-55,w-420,22),juce::Justification::centred,1);
}

void LTASoundbankAudioProcessorEditor::drawPlay(juce::Graphics& g)
{
    const int w=getWidth(),h=getHeight();
    auto frame=juce::Rectangle<float>(26.0f,87.0f,(float)w-52,(float)h-175);
    g.setColour(panel);
    g.fillRoundedRectangle(frame,22.0f);
    g.setColour(juce::Colour(0xff40334b));
    g.drawRoundedRectangle(frame,22.0f,1.0f);
    const int selected=processor.selectedPreset();
    const auto& preset=LTASoundbankAudioProcessor::presets()[(size_t)selected];
    const juce::String cat=LTASoundbankAudioProcessor::categoryNames[(size_t)preset.category];
    drawPill(g,{52,105,165,33},cat.toUpperCase(),true);
    drawPill(g,previewRect,"PLAY NOTE  \xE2\x96\xB6");
    // Real audio signal, horizontal scrolling response, no invented visual spectrum.
    const float cy=(float)waveRect.getCentreY();
    const float left=(float)waveRect.getX(), width=(float)waveRect.getWidth();
    const float height=(float)waveRect.getHeight();
    const float amp=juce::jlimit(0.0f,1.0f,visualLevel*2.2f);
    const int head=processor.scopePosition();
    const juce::Colour primary=preset.category==0?juce::Colour(0xffb2a7ff):
        preset.category==6?juce::Colour(0xffffc1dc):
        preset.category==4?juce::Colour(0xffabbcff):accent;
    // central subtle grid
    g.setColour(juce::Colour(0xff453a50).withAlpha(0.32f));
    for(int i=1;i<=3;++i)
    {
        const float y=(float)waveRect.getY() + (float)i*height/4.0f;
        g.drawHorizontalLine(juce::roundToInt(y),left,left+width);
    }
    g.setColour(lilac.withAlpha(0.18f));
    g.drawLine(left,cy,left+width,cy,1.5f);
    juce::Path wave;
    const int points=300;
    for(int i=0;i<points;++i)
    {
        const int at=head-((points-i)*512/points);
        const float sample=processor.scopeSample(at);
        const float x=left+width*(float)i/(float)(points-1);
        const float y=cy-sample*(height*0.63f);
        if(i==0) wave.startNewSubPath(x,y);
        else wave.lineTo(x,y);
    }
    g.setColour(primary.withAlpha(0.10f+amp*0.13f));
    g.strokePath(wave,juce::PathStrokeType(15.0f));
    g.setColour(primary.withAlpha(0.22f+amp*0.2f));
    g.strokePath(wave,juce::PathStrokeType(5.0f));
    g.setColour(primary.withAlpha(0.85f));
    g.strokePath(wave,juce::PathStrokeType(2.3f));
    if(visualLevel<0.001f)
    {
        g.setColour(muted.withAlpha(0.7f));
        g.setFont(juce::Font(juce::FontOptions(15.0f)));
        g.drawText("Play a note to bring this sound to life",waveRect,
            juce::Justification::centred);
    }
    const int titleY=h-180;
    g.setColour(ink);
    g.setFont(juce::Font(juce::FontOptions(29.0f).withStyle("Bold")));
    g.drawFittedText(preset.name,juce::Rectangle<int>(160,titleY,w-320,39),juce::Justification::centred,1);
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    juce::String tag;
    if(preset.category==0) tag="808 / SUB / RAGE";
    else if(preset.category==1) tag="LEAD / SYNTH / UNDERGROUND";
    else if(preset.category==2) tag="PLUGG / BOUNCY / DREAMY";
    else if(preset.category==3) tag="KEYS / EMOTIONAL / NOSTALGIC";
    else if(preset.category==4) tag="PAD / AMBIENT / CLOUD RAP";
    else if(preset.category==5) tag="ARP / RHYTHM / PLUGG";
    else if(preset.category==6) tag="SYNTHETIC VOX / CHOIR";
    else if(preset.category==7) tag="SYNTH STRINGS / ATMOSPHERIC";
    else tag=selected%50<25?"ELECTRIC-STYLE GUITAR":"ACOUSTIC-STYLE GUITAR";
    g.drawText(tag,160,titleY+39,w-320,19,juce::Justification::centred);
    drawPill(g,backRect,"<");
    drawPill(g,nextRect,">");
    drawPill(g,favRect,processor.isFavourite(selected)?"FAV \xE2\x99\xA5":"FAV +",processor.isFavourite(selected));
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText(juce::String(selected+1)+" / 400",w/2-70,h-115,140,36,juce::Justification::centred);
}

void LTASoundbankAudioProcessorEditor::drawBrowser(juce::Graphics& g)
{
    g.setColour(panel);
    g.fillRoundedRectangle(browserRect.toFloat(),20.0f);
    const int x=categoryRect.getX(),y=categoryRect.getY();
    // Left/right category chips, plus ALL/Favourites.
    const int count=11;
    const float tab=(float)categoryRect.getWidth()/(float)count;
    for(int j=0;j<count;++j)
    {
        const juce::String name=j==0?"ALL":j==10?"FAVS":
             juce::String(LTASoundbankAudioProcessor::categoryNames[(size_t)j-1]);
        const bool chosen=j==0?activeCategory==-1&&!favOnly:
            j==10?favOnly:activeCategory==j-1&&!favOnly;
        const juce::Rectangle<int> r((int)(x+j*tab),y,(int)tab-3,34);
        drawPill(g,r,name,chosen);
    }
    const int columns=4,rows=4;
    const float gap=10;
    const float cw=((float)gridRect.getWidth()-gap*(columns-1))/columns;
    const float ch=((float)gridRect.getHeight()-gap*(rows-1))/rows;
    const auto& all=LTASoundbankAudioProcessor::presets();
    for(int j=0;j<columns*rows;++j)
    {
        const int matchIndex=firstVisible+j;
        if(matchIndex>=(int)matches.size()) break;
        const int idx=matches[(size_t)matchIndex];
        const int col=j%columns,row=j/columns;
        const juce::Rectangle<float> r((float)gridRect.getX()+col*(cw+gap),
            (float)gridRect.getY()+row*(ch+gap),cw,ch);
        const bool selected=idx==processor.selectedPreset();
        g.setColour(selected?juce::Colour(0xff5b406b):panel2);
        g.fillRoundedRectangle(r,12.0f);
        if(selected)
        {
            g.setColour(accent);
            g.drawRoundedRectangle(r,12.0f,2.0f);
        }
        g.setColour(selected?accent:muted);
        g.setFont(juce::Font(juce::FontOptions(11.0f)));
        g.drawText(LTASoundbankAudioProcessor::categoryNames[(size_t)all[(size_t)idx].category],
            r.toNearestInt().reduced(13,9).removeFromTop(17),juce::Justification::centredLeft);
        g.setColour(ink);
        g.setFont(juce::Font(juce::FontOptions(16.0f).withStyle("Bold")));
        g.drawFittedText(all[(size_t)idx].name,r.toNearestInt().reduced(12,26),juce::Justification::centred,2);
        g.setColour(processor.isFavourite(idx)?accent:muted);
        g.setFont(juce::Font(juce::FontOptions(11.0f)));
        g.drawText(juce::String(idx+1)+ (processor.isFavourite(idx)?"  *":""),
            (int)r.getX()+13,(int)r.getBottom()-25,(int)r.getWidth()-26,18,juce::Justification::centredLeft);
    }
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    const int top=gridRect.getBottom()+5;
    g.drawText(juce::String(matches.size())+" sounds found  |  Scroll for more  |  Click to load  |  Double-click to play",
        45,top,getWidth()-90,19,juce::Justification::centred);
}

void LTASoundbankAudioProcessorEditor::rebuildList()
{
    matches.clear();
    auto query=search.getText().trim().toLowerCase();
    const auto& all=LTASoundbankAudioProcessor::presets();
    for(int idx=0;idx<400;++idx)
    {
        const auto& p=all[(size_t)idx];
        if(activeCategory>=0 && p.category!=activeCategory) continue;
        if(favOnly && !processor.isFavourite(idx)) continue;
        const juce::String category=LTASoundbankAudioProcessor::categoryNames[(size_t)p.category];
        if(query.isNotEmpty() && !p.name.toLowerCase().contains(query) &&
           !category.toLowerCase().contains(query) &&
           !(query=="cloud" && (p.category==2||p.category==4)) &&
           !(query=="rage" && (p.category==0||p.category==1)) &&
           !(query=="plugg" && (p.category==2||p.category==5))) continue;
        matches.push_back(idx);
    }
    firstVisible=juce::jlimit(0,std::max(0,(int)matches.size()-16),firstVisible);
}

void LTASoundbankAudioProcessorEditor::choosePreset(int idx)
{
    if(idx<0 || idx>=400) return;
    processor.selectPreset(idx);
    transition=1.0f;
    lastPreset=idx;
    repaint();
}

int LTASoundbankAudioProcessorEditor::positionToPreset(juce::Point<int> pt) const
{
    if(!gridRect.contains(pt)) return -1;
    const int columns=4,rows=4;
    const float gap=10;
    const float cw=((float)gridRect.getWidth()-gap*(columns-1))/columns;
    const float ch=((float)gridRect.getHeight()-gap*(rows-1))/rows;
    const int col=juce::jlimit(0,columns-1,(int)((pt.x-gridRect.getX())/(cw+gap)));
    const int row=juce::jlimit(0,rows-1,(int)((pt.y-gridRect.getY())/(ch+gap)));
    const int index=firstVisible+row*columns+col;
    return index<(int)matches.size()?matches[(size_t)index]:-1;
}

void LTASoundbankAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    const auto pt=e.getPosition();
    if(browseRect.contains(pt))
    {
        browsing=!browsing;
        search.setVisible(browsing);
        if(browsing) rebuildList();
    }
    else if(surpriseRect.contains(pt))
    {
        rebuildList();
        const int index=matches.empty()?juce::Random::getSystemRandom().nextInt(400):
            matches[(size_t)juce::Random::getSystemRandom().nextInt((int)matches.size())];
        choosePreset(index);
        browsing=false;
        search.setVisible(false);
    }
    else if(!browsing)
    {
        const int selected=processor.selectedPreset();
        if(backRect.contains(pt)) choosePreset((selected+399)%400);
        else if(nextRect.contains(pt)) choosePreset((selected+1)%400);
        else if(previewRect.contains(pt)) processor.requestPreview();
        else if(favRect.contains(pt)) processor.setFavourite(selected,!processor.isFavourite(selected));
    }
    else
    {
        if(categoryRect.contains(pt))
        {
            const float tab=(float)categoryRect.getWidth()/11.0f;
            const int j=juce::jlimit(0,10,(int)((pt.x-categoryRect.getX())/tab));
            favOnly=j==10;
            activeCategory=(j==0||j==10)?-1:j-1;
            firstVisible=0;rebuildList();
        }
        else if(auto idx=positionToPreset(pt);idx>=0)
        {
            choosePreset(idx);
            if(e.getNumberOfClicks()>1) processor.requestPreview();
        }
    }
    repaint();
}

void LTASoundbankAudioProcessorEditor::mouseWheelMove(const juce::MouseEvent& e,
    const juce::MouseWheelDetails& wheel)
{
    if(browsing && gridRect.contains(e.getPosition()))
    {
        firstVisible=juce::jlimit(0,std::max(0,(int)matches.size()-16),
            firstVisible+(wheel.deltaY>0?-4:4));
        repaint();
    }
    else juce::AudioProcessorEditor::mouseWheelMove(e,wheel);
}

bool LTASoundbankAudioProcessorEditor::keyPressed(const juce::KeyPress& key)
{
    if(!browsing && key.getKeyCode()==juce::KeyPress::leftKey)
    { choosePreset((processor.selectedPreset()+399)%400);return true; }
    if(!browsing && key.getKeyCode()==juce::KeyPress::rightKey)
    { choosePreset((processor.selectedPreset()+1)%400);return true; }
    return false;
}

void LTASoundbankAudioProcessorEditor::timerCallback()
{
    const int current=processor.selectedPreset();
    if(current!=lastPreset){lastPreset=current;transition=1.0f;}
    const float level=processor.outputLevel();
    visualLevel=visualLevel*0.84f+level*0.16f;
    transition*=0.87f;
    ++animationFrame;
    repaint();
}
