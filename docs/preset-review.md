# Preset review — 2026-11-03

Historical review of all 74 presets in version 2.0.1. The findings and measurements
below describe that snapshot; see the implemented corrections in version 2.0.2 here.

## Implemented in 2.0.2

- Preset 21 now explicitly selects the triangle envelope zone (COLOR 256). This
  deliberately changes its previously duplicated audio; preset 42 retains the saw.
- Misleading names were corrected without changing the other preset definitions:
  sustained bells are now FIFTH/HARM voices, the SID pulse-plus-noise hit is named
  SID PULSE HIT, slow PWM is SID GENTLE PWM, and TIA's low-rate noise is not labelled a hat.
- Family names use PSG, YM and SID consistently; mixed waves, hybrid WT4 voicing,
  and musical arpeggio divisions are labelled explicitly. All 74 IDs are preserved.
- Gain boosts, new percussion envelopes, bass retuning and other audition-dependent
  proposals remain deferred. Compatibility/experimental patches remain selectable.
- The numerical measurements below have not been regenerated for the corrected
  triangle. New regression cases compare triangle/saw audio and triangle pitch at
  55, 220 and 880 Hz. The duplicate was reproduced before the correction; all three
  distinct-waveform checks pass afterward. Both suites pass with undefined-behavior
  sanitization (332 core and 760 adapter checks). The preset catalog lists the current names.

## Recommendation

Keep 57 presets in the main curated collection and classify 17 as compatibility
or experimental material: TIA 14/28/51/52, FDS 33/34/35/54/55/56/73, and OLD PCM
36/37/38/57/58/59. These are editorial groups, not a newly implemented menu.
The current contiguous numeric PRESET menu cannot simply hide arbitrary IDs.

Do not physically remove or renumber entries in a compatible release. Preserve
the original sound for archived entries. Replacing a patch under the same ID
can also change recalled sounds, particularly hidden preset properties such as
vibrato. Publish deliberate sound changes with migration notes; append a new ID
when preserving the old patch matters. Cosmetic renames are less disruptive.

The strongest deletion/merge candidate is **21 ATARI TRI**, which is currently
an exact duplicate of **42 ATARI SAW**. Prefer repairing 21 into a real triangle
envelope patch rather than losing a useful instrument category.

## Evidence and limits

- Rendered every preset at 55, 220, and 880 Hz, full velocity, 48 kHz, default
  tempo, and no external modulation: 0.6 seconds held, 0.2 seconds released.
- Compared all preset pairs for sample-exact equality at all three pitches.
  **Only 21/42 were identical in this test.** This is not proof that all other
  pairs are perceptually distinct.
- Compared COLOR endpoints and DETN/FLT endpoints on separate 0.4-second renders
  at 220 Hz. Zero difference means those endpoints produced identical samples;
  source inspection supplies the explanation for the inactive controls below.
- Measured RMS over 50–500 ms at 220 Hz. This is an engineering measurement,
  not perceived loudness or LUFS. Decaying percussion should not be balanced
  against sustained sounds using this window.
- Temporary measurements and the audit harness are in
  `build/preset-review/metrics.csv` and `build/preset-review/audit.cpp`.
  They are ignored build artifacts and can be regenerated while retained locally.
- This review uses source inspection and rendered-signal measurements, not a
  subjective listening session or hardware audition. Suggested numerical tweaks
  are starting points for A/B listening, not validated final voicings.

## Highest-priority refinements

1. **Repair the triangle/saw duplicate.** Both 21 and 42 resolve to COLOR 128,
   layered voice mode, and the same rising envelope. In layered buzz mode COLOR
   overrides the saw/triangle distinction in SUB. Start 21 at COLOR 256 (triangle
   zone); keep 42 as the saw reference. Check pitch and waveform after recall.
2. **Make the bell names describe their envelopes.** WT4 BELL 29, X FDS BELL 33,
   and PCE BELL 48 all have DECAY 0 and sustain until note-off. Audition DECAY
   8–12, or rename them to sustained FM/harmonic voices. OLD PCM BELL 59 already
   has a decay envelope and does not have this particular problem.
3. **Correct percussion roles.** TIA KICK 14 is noise-only, with no tonal body or
   falling pitch envelope. Rename it to X TIA THUD/BURST, or build a genuinely
   different kick with a tonal layer and pitch envelope. TIA HAT 51 uses the
   lowest of these four TIA noise settings; it needs a faster noise rate and a
   shorter decay to serve as a bright closed hat.
4. **Clarify C64 NOISE 23.** It currently mixes two pulse oscillators with noise.
   Select VOICE NOISE if the intent is a noise-only hit; otherwise rename it
   SID PULSE HIT. Keep the resonant/filter character distinct from GB/NES noise.
5. **Make slow PWM achievable and accurately named.** SID SLOW PWM 66 resolves
   to about 1.05 Hz with depth 0.056, versus about 4.04 Hz/depth 0.226 for the
   ordinary pulse macro midpoint. Rename it SID GENTLE PWM now. A broad 0.2–0.4 Hz
   sweep would require decoupling PWM speed and depth in the macro design; simply
   lowering COLOR also collapses its modulation depth.
6. **Balance sustained patches before adding more.** At the measured pitch,
   YM ENV SOLO 65 is -19.90 dBFS RMS, while NES OCT ARP 40 is -4.59 dBFS RMS.
   WT4 SAW 30 is -15.75 dBFS and WT4 ORGAN 31 is -4.79 dBFS. Use representative
   bass/lead notes and matched musical phrases before choosing gain trims.
   Avoid blindly boosting the quieter patches: some already have high peaks.

## Complete preset disposition

“Keep” preserves the role; it still participates in bank-wide naming and level
checks. “Refine” identifies a specific patch improvement. “Archive” preserves the
ID and sound as compatibility/experimental material, outside the main recommendation
list. Control names below refer to the exposed UI unless a core field is named.

| ID | Current preset | Decision | Proposed refinement or rationale |
|---|---|---|---|
| 0 | NES LEAD | Keep | Useful detuned lead with delayed vibrato; use as the NES lead reference. |
| 1 | NES ARP | Keep | Preserve the short, 60 Hz major tracker stab; distinguish it from tempo-synced 71. |
| 2 | NES BASS | Refine | Audition TUNE MUSIC for deep bass; CHIP mode folds notes below register limits. Compare level against other bass patches. |
| 3 | GB WAVE | Refine | Rename GB WAVE MIX: it includes two pulses and a lower wave layer, not a wave-only organ. Keep 62 as the isolated wave. |
| 4 | SEGA BUZZ | Keep | Rename PSG BUZZ; square/sub plus pitched periodic noise has a clear role. |
| 5 | ATARI BUZZ | Refine | Rename YM NOISE ENV. Audition reduced noise gating or a dry envelope alternative; distinguish it from clean 42/65. |
| 6 | C64 PWM | Keep | Rename SID PWM; preserve the resonant PWM-plus-sub reference. |
| 7 | C64 SYNC | Keep | Rename SID SYNC; one of the bank's distinct synthesis modes. |
| 8 | C64 RING | Keep | Rename SID RING; preserve the fifth-related ring texture. COLOR currently has no effect in ring mode. |
| 9 | PCE FM | Keep | Keep the sustained octave-ratio FM voice; make 48 the contrasting decaying instrument. |
| 10 | POKEY BASS | Keep | Keep the polynomial tone plus clean lower voice; check bass-range tuning in both TUNE modes. |
| 11 | POKEY HPF | Keep | Preserve the high-pass interaction; it is more than a waveform variation. |
| 12 | VRC6 LEAD | Keep | Keep the narrow pulses plus lower saw layer and vibrato. |
| 13 | YM 5B BUZZ | Refine | Rename YM5B ENV HIT. It is a short, noise-gated envelope, not a clean triangle; audition NOISE OFF or a longer DECAY 6–8. |
| 14 | X TIA KICK | Archive | Noise-only burst, not a pitched kick. Rename X TIA THUD or retain as documented legacy percussion. |
| 15 | NES CHORUS | Keep | Clear detuned sustained alternative to 0; no delayed vibrato. |
| 16 | NES SNARE | Refine | Noise-only hit is valid. Audition a small tonal body only if it materially improves the snare role; otherwise document it as noise snare. |
| 17 | NES MINOR | Refine | Unlike major 1, it sustains. Rename NES MIN RUN, or audition DECAY 4 for a matched major/minor stab pair. |
| 18 | GB HIHAT | Refine | Audition NOISE METAL and DECAY 1 instead of MID/6 for a shorter metallic hat. HIGH is another option, but HIGH/DECAY 2 would duplicate noise hit 63. |
| 19 | SEGA LEAD | Keep | Rename PSG FIFTH: the fixed fifth is the defining feature, with detune and vibrato secondary. |
| 20 | ATARI LEAD | Keep | Rename YM LEAD. Preserve the detuned square stack; document that COLOR is inactive in this tone-only mode. |
| 21 | ATARI TRI | Refine | Exact duplicate of 42. Set COLOR into the triangle zone, initially 256, and rename YM TRI ENV. Retire only if not repaired. |
| 22 | C64 BASS | Keep | Rename SID BASS; retain the filtered saw/sub contrast to NES and PCE basses. |
| 23 | C64 NOISE | Refine | Select VOICE NOISE for a real noise-only preset, or rename SID PULSE HIT to describe the existing layers. |
| 24 | C64 ARP | Refine | Rename SID ARP; audit its strong layered level. Keep 60 Hz tracker behavior or explicitly name a new tempo-synced variant. |
| 25 | PCE ORGAN | Refine | Reduce detune from 2 toward 0–0.5 for a steadier organ; it mixes a sine-like main wave and an octave saw, so audition harmonic balance. |
| 26 | POKEY LEAD | Keep | Useful pure-tone lead with detune/vibrato; complements the polynomial voices. |
| 27 | VRC6 ARP | Keep | Preserve tracker arp with a lower saw foundation; distinct from the NES arp timbre. |
| 28 | X TIA SNARE | Archive | Preserve the fast-noise burst as experimental percussion; compare against 14/51/52 before expanding this set. |
| 29 | WT4 BELL | Refine | DECAY is currently off. Audition 8–12 and less detune, or rename WT4 FIFTH for the sustained fifth-related blend. |
| 30 | WT4 SAW | Refine | Rename WT4 SAW MIX; the second wave is not another saw. Investigate phase cancellation and level before applying gain. |
| 31 | WT4 ORGAN | Refine | One of the loud sustained patches. Audition reduced secondary/lower layer gains while retaining the organ identity. |
| 32 | WT4 BUZZ | Refine | The second voice crosses into the five-bit wave bank. Decide whether to keep and name that hybrid or constrain secondary-wave selection within the bank. |
| 33 | X FDS BELL | Archive | Preserve legacy audio; a future bell revision needs DECAY 8–12 or an accurate sustained-harmonic name. |
| 34 | X FDS BASS | Archive | Deep experimental FM plus lower voice; hardware CPU and musical tuning need audition before promotion. |
| 35 | X FDS WOBBLE | Archive | Retain as the strongest deep-modulation FDS demonstration; compare with 73 before retaining both in a curated experimental subset. |
| 36 | OLD PCM SAW | Archive | Generated-wave compatibility voice; low priority beside PCE/WT saws. Do not present it as a new sampled instrument. |
| 37 | OLD PCM FM | Archive | Preserve FM behavior for old patches; a candidate to retain in a small legacy showcase. |
| 38 | OLD PCM AM | Archive | Preserve the distinctive AM mode; stronger justification than another plain legacy saw. |
| 39 | NES FIFTH | Keep | Straight power-chord reference; useful alongside detuned and arpeggiated voices. |
| 40 | NES OCT ARP | Refine | Strongest measured sustained RMS in the bank. Audition a lower mix level or DECAY 4–6; retain octave tracker movement. |
| 41 | SEGA BASS | Refine | Rename PSG BASS and audition TUNE MUSIC. The PSG divider cannot retain deep bass fundamentals in CHIP mode. |
| 42 | ATARI SAW | Keep | Rename YM SAW ENV; retain as the layered saw counterpart after repairing 21. |
| 43 | ATARI ARP | Keep | Rename YM SUS ARP to expose its suspended pattern. COLOR is inactive in the current tone-only configuration. |
| 44 | C64 TRI | Keep | Rename SID TRI; keep the filtered triangle as a softer SID alternative. Compare levels over a longer unison-beating cycle. |
| 45 | C64 HOLLOW | Keep | Rename SID HOLLOW; filtered pulse fills a clear role beside the saw bass. |
| 46 | PCE BASS | Refine | Audition TUNE MUSIC so the lower layer remains a true octave at low notes; keep its saw/pulse character. |
| 47 | PCE ARP | Keep | Rename PCE MAJ TRK; distinguish its fast major pattern from tempo-synced minor 72. |
| 48 | PCE BELL | Refine | Add DECAY 8–12 and audition a more controlled FM depth; reserve 9 for sustained FM. Rename if it remains sustained. |
| 49 | POKEY BUZZ | Keep | Preserve the gated-polynomial blend with lower tone; 69 remains the cleaner isolated comparison. |
| 50 | YM 5B BASS | Keep | Rename YM5B BASS; retain the Sunsoft-clock bass reference. COLOR currently does nothing in this tone-only mode. |
| 51 | X TIA HAT | Archive | Current low noise rate and DECAY 4 do not support a bright, short hat role; preserve or rename, rather than claiming a refined hat. |
| 52 | X TIA METAL | Archive | ARP MAJ is inaudible on the fixed-rate noise-only voice. Remove that misleading setting in a revised patch; keep the old ID compatible. |
| 53 | WT4 ARP | Refine | Keep a minor pluck, but audition DECAY 5–6 instead of 3 for a clearer musical phrase; compare attack energy separately from sustained patches. |
| 54 | X FDS BRASS | Archive | Steady FM saw has no brass-style attack articulation. Rename X FDS SAW or develop an envelope before promoting it as brass. |
| 55 | X FDS KEYS | Archive | Existing decay makes the keys role credible; a good candidate for a small experimental showcase. |
| 56 | X FDS ARP | Archive | Preserve FM tracker behavior; do not remove merely because 33 has similar average level. |
| 57 | OLD PCM BASS | Archive | Preserve compatibility; the main collection already supplies several clearer bass references. |
| 58 | OLD PCM CHORD | Archive | Rename only with migration care: its defining motion is a minor arpeggio, not a static three-note chord. |
| 59 | OLD PCM BELL | Archive | Already decays. Keep as legacy material rather than introducing another main-bank bell. |
| 60 | NES SOLO | Keep | Useful isolated pulse reference. DETN is correctly inactive because voice 2 is absent. |
| 61 | NES TRI | Keep | Essential isolated triangle reference; both front-panel macros are currently inactive. |
| 62 | GB WAVE SOLO | Refine | Rename GB ORGAN SOLO for the selected table, or keep generic name as a wave-browser starting point. Assess level before boosting. |
| 63 | GB NOISE | Refine | Rename GB NOISE HIT; preserve the very short high-noise burst and differentiate it from revised closed-hat 18. |
| 64 | PSG METAL | Refine | Rename PSG PERIODIC or PSG BUZZ HIT: this is pitched periodic noise. Audition DECAY 6–8; evaluate its attack separately from RMS over 500 ms. |
| 65 | YM ENV SOLO | Keep | Rename YM SAW SOLO; useful pure envelope reference. It is quieter than layered patches; gain changes require peak and noise checks. |
| 66 | SID SLOW PWM | Refine | Rename SID GENTLE PWM for current behavior. A slower, deeper sweep requires a macro redesign, not only a COLOR edit. |
| 67 | PCE WAVE | Keep | Rename PCE TRI SOLO; isolated triangle wave is a clear baseline. DETN is inactive with P2 muted. |
| 68 | WT4 SOLO | Keep | Rename WT4 BELL SOLO or WT4 HARM SOLO; preserve the isolated four-bit harmonic wave without implying a decaying envelope. |
| 69 | POKEY POLY | Keep | Preserve as the isolated polynomial reference; DETN is inactive with P2 muted. |
| 70 | VRC6 SAW | Keep | Rename VRC6 SAW SOLO; keep the isolated saw despite inactive COLOR and DETN. |
| 71 | NES SYNC ARP | Refine | Rename NES MAJ 16TH to make tempo sync unambiguous; compare its high level with 40 and reduce layer balance if needed. |
| 72 | PCE SYNC ARP | Keep | Rename PCE MIN 16TH; the minor tempo-synced pattern is distinct from 47. |
| 73 | X FDS TABLE | Archive | Generic name hides its role. Compare with 35 and, if retained, label the particular FM character instead of the implementation. |

## Controls and implementation refinements

Several patch limitations require engine or UI work rather than preset edits:

- **Inactive COLOR endpoints:** 8, 14, 16, 18, 20, 28, 43, 50, 51, 52, 61, 63,
  64, 65, 70. Ring mode fixes its waveform; pure envelopes fix their shape; solo
  noise/triangle/saw sources bypass the pulse/wave macro; tone-only YM patches
  do not use the envelope macro. These are not reasons to delete useful solo
  presets, but the controls should be documented accurately.
- **Inactive DETN/FLT endpoints:** 14, 16, 18, 28, 51, 52, 60, 61, 62, 63, 64,
  65, 67, 68, 69, 70. The register-detune path mainly affects a second tone or
  modulator; solo sources and fixed-rate noise therefore have no destination.
  A future source-aware macro could use noise rate, wave level, or envelope time,
  but that changes the control contract and needs save/recall tests.
- **Noise-only pitch independence:** 14, 16, 18, 28, 51, 52, and 63 produced the
  same samples at 55 and 880 Hz. That is reasonable for fixed percussion, but
  inconsistent with pitched-kick promises or active-looking arpeggio settings.
  PSG periodic noise 64 does track the note and should remain a separate case.
- **Mixed PCE wave banks:** the secondary table uses `(table + 1) & 7`. COLOR's
  last wave in either bank therefore mixes with the first wave of the other bank.
  This affects 32 and the layered PCE triangle presets 47/72. A within-bank wrap
  would change established sounds, so decide explicitly and test compatibility.
- **Readable, explicit preset definitions:** move COLOR, VOICE, arp rate, and
  variant choices into named preset data rather than relying on scattered
  `if (index == ...)` overrides. Add named IDs. Preserve serialized values and
  verify sample identity before treating this as a behavior-neutral refactor.
- **Naming:** use NES, GB, PSG, YM, YM5B, SID, PCE, WT4, POKEY, and VRC6 consistently.
  Reserve `SOLO`, `HIT`, `TRK`, and `16TH` for real differences. Check display
  truncation on hardware before adopting longer proposed labels.

## Acceptance criteria for a refinement pass

1. Preserve IDs; document which names or sounds intentionally change.
2. Repair 21/42 and confirm distinct waveforms at multiple pitches with correct tuning.
3. Give bell/hat/noise labels measurable matching envelope and source behavior.
4. Compare basses at E1/A1 as well as midrange notes, with CHIP and MUSIC tuning.
5. Audition sustained sounds at matched perceived level; compare percussion in
   a separate transient window. Measure peaks after any gain or layer change.
6. Verify preset-only recall, all-parameter recall in both orders, repeated edits,
   and engine changes. Hidden gain/PWM properties must survive serialization.
7. Run the existing audio and adapter suites, then check the final bank on hardware.
