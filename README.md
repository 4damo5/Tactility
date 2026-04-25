# Tactility: The Portable Braille Reader
*Knowledge at Your Fingertips* · Forge Hardware Spring 2026 · Northeastern University

---

Tactility is a handheld Braille reading device that converts digital text into tactile output in real time. Existing refreshable Braille displays cost hundreds to thousands of dollars and are rarely portable, while Tactility is designed to be compact and affordable.

## The Team

| Name | Team |
|---|---|
| Adam Otsuka | Lead |
| Alexandre Lannibois | ECE |
| Daniel Brightman | ECE |
| Ibrahim Rogers | ME |
| Mary Morris | ECE |
| Ryan Heberlig | ME |
| Supratim Shaud | ME |

## How It Works

A rack-and-pinion mechanism drives 6 pins per Braille cell using N20 gearbox motors. An ESP32 microcontroller runs a PDF → text → Braille pipeline, mapping each character to a 6-pin binary array and actuating the motors accordingly. Navigation is handled via scroll wheel, push buttons, and vibration feedback. The PCB was designed in Altium.

## Prototypes

Five iterations were built over the semester, starting with a breadboard proof-of-concept and ending with a 2-cell showcase model with a custom PCB. The ideal form factor is a snap-fit handheld case with Tactility's orange-and-white branding.

## Future Work

- Electromagnetic pin system for a smaller footprint
- MagSafe induction charging for single-hand use
- Phone case form factor
- Companion app for cross-platform text input
