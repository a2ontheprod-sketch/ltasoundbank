# LTA SOUNDBANK — VST3 instrument | v1.0 (prototype)

An original JUCE VST3 synthesiser instrument with 400 algorithmically generated preset configurations (50 in each of nine categories), real-time waveform, library browser, favourites, audition and Surprise Me.

**This is an initial synthesised soundbank, not 400 individually recorded or human-auditioned professional sounds.** Every preset uses a different combination of sound parameters and some instrument families have distinct synthesis rules. Synthetic choir, string and guitar presets do not contain realistic recorded instruments. An actual commercial soundbank would require detailed sound-design, listening QA and licensed multisamples. A Windows/FL Studio compile has not been completed by this package creator; GitHub Actions must verify it.

## Repository upload (do not upload the enclosing ZIP)
1. Extract this ZIP to your PC.
2. Go to https://github.com/new and create `lta-soundbank` (public or private).
3. On the new repo choose **Add file > Upload files**. Upload `CMakeLists.txt`, the `Source` folder, `README_START_HERE.md` and `GITHUB_WORKFLOW_COPY_PASTE.yml` from the extracted project. Commit changes.
4. Ensure `.github/workflows/build-windows.yml` is present in GitHub. If the hidden `.github` directory was not uploaded, use **Add file > Create new file**, type `.github/workflows/build-windows.yml` as the file name and paste the entire content of `GITHUB_WORKFLOW_COPY_PASTE.yml`. Commit it.
5. Go to **Actions > Build LTA Soundbank VST3 > Run workflow > Run workflow**. A push to `main` may also run it automatically. Open the latest run.
6. When the run is green, scroll to **Artifacts** and download `LTA-SOUNDBANK-Windows-VST3`. Unzip it, then unzip `LTA_SOUNDBANK_Windows_VST3.zip` if present.
7. Close FL Studio. Copy the **entire** `LTA Soundbank.vst3` folder to `C:\Program Files\Common Files\VST3` (approve Windows UAC if prompted).
8. Open FL Studio > Options > Manage plugins > Find installed plugins. Add **LTA Soundbank** as a **Channel Rack instrument**, not as a Mixer effect.
9. Load a Piano Roll MIDI melody. Use the PLAY NOTE control to audition the currently chosen preset without MIDI. Browse, favourites and randomise from the plugin. Save and reopen an FL Studio project to test preset restoration.

## Notes / limitations
- 400 distinct parameter configurations, **not** 400 handcrafted recordings or artist-approved presets. Category diversity is a starting point for sound-design QA.
- 25 electric-style and 25 acoustic-style synthesised guitar patches in the 50-guitar category.
- 50 arpeggiator presets react to held MIDI notes and follow host BPM. Arpeggios are generated inside the synth, and the visual is a **waveform of the actual output**.
- Sound settings themselves are fixed in this version; no sound-edit knobs.
- Favourites and selected preset save with plugin state in the project (not a global favourites database).
- Performance, host automation, polyphony, gain and CPU should be tested in FL Studio before distributing this publicly.
- Clicking PLAY NOTE auditions middle C (bass audition uses C2) for a short time.
- If GitHub Actions fails, open the red Build VST3 step and inspect the **first actual C++ error**.
