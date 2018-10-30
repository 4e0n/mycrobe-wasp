# mycrobe-wasp
Microcontroller-based batch audio presenter -- **currently the development continues in debug branch!**

# Short history

mycrobe-wasp is the code for a device I manufactured during my Neuroscience postdoc between 2010-2012 at CerCo/CNRS, Toulouse between for a project where we needed to simultaneously present auditory and visual stimuli. People working on especially with EEG and Event-related Electrophysiology can imagine the problems the jitter between two modalities can cause, so this device simply by-passes any e.g. DMA-related issue, meaning that you're alone with only the jitter of parallel port access.

The device is based upon a small PIC32-based embedded board from MikroElektronika and expects a TTL-trigger from a computer (in our case it was parallel port driven within Matlab & PsychToolbox). In each trigger it plays .wav files (which are pre-arranged to fit in your Psychophysical trial set) on its microSD card one-by-one, also displaying which file currently it's playing on its color LCD-screen. So here resides its codebase ready to compile in PIC32 version of MikroC. Here are some pictures I've taken during development (others probably will arrive in time):

Some oscilloscope output demonstrating the jitter level:
<div align="center">
<img src="http://icon.unrlabs.org/projects/mycrobe-wasp/images/wasp0.png" width="300">
</div>
<div align="center">
<img src="http://icon.unrlabs.org/projects/mycrobe-wasp/images/wasp1.png" width="300">
</div>

The device itself:

<div align="center">
<img src="http://icon.unrlabs.org/projects/mycrobe-wasp/images/wasp2.png" width="300">
</div>
