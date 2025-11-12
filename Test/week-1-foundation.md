# Week 1: Building the Foundation

**I'm building a game engine from scratch and here's what I learned this week.**

Ever wondered what it takes to render a single triangle on screen? Turns out, a lot more than you'd think.

I'm working on Ignis—a 3D game engine written entirely in C with Vulkan handling the graphics. No Unity, no Unreal, just raw code and the Vulkan API staring back at me.

**Why am I doing this?**

Because lower level systems have always intrigued me and understanding how memory management, event systems, and GPU pipelines actually work? That's what really pushed me to begin working on this mountainous project.

**What I've built so far:**

The foundation is solid. I've got cross-platform window creation working on both Windows and Linux. The event system routes input across the engine. Memory allocations are tracked with 19 different tags so I can catch leaks before they become nightmares.

The input system uses double-buffered state (fancy way of saying it remembers what happened last frame) so games can detect "just pressed" vs "held down" accurately.

**The interesting part:**

I'm doing this in pure C—no C++, no templates, no runtime overhead. Everything is explicit. When I say "create a dynamic array," I know exactly what's happening in memory. Vulkan's the same way: explicit control over the GPU, no magic happening behind the scenes.

This week I finished the device selection and queue management for Vulkan. Translation: the engine can now talk to your graphics card and tell it what to do.

**What's next:**

The swapchain. Then render passes. Then the graphics pipeline.

My goal? Get a triangle on screen by next week.

Sounds simple, but that triangle represents dozens of hours wrestling with Vulkan's verbosity. Once it renders, everything else builds on that foundation—textures, 3D models, lighting, the works.

**The takeaway:**

Building from scratch is hard. But it's the fastest way to understand what's actually happening when you click "Play" in a game engine.

If you're curious about graphics programming or engine architecture, I'll be posting weekly updates as I work through this.

The project is public, the link is below.

Thanks for reading through!

---

#GameDev #VulkanAPI #EngineArchitecture #GraphicsProgramming #SystemsProgramming


--This is what I actually posted on LinkedIn

𝗪𝗲𝗲𝗸 𝟲: 𝗦𝘁𝗶𝗹𝗹 𝘁𝗿𝘆𝗶𝗻𝗴 𝘁𝗼 𝗱𝗿𝗮𝘄 𝗼𝗻𝗲 𝘁𝗿𝗶𝗮𝗻𝗴𝗹𝗲 😂

I’ve spent the last 6 weeks building a 3D game engine from scratch.
No Unity.
 No Unreal.
 Just me, C, Vulkan, and a questionable life decision.

Say hello to Ignis — my game engine that's currently powerful enough to... almost maybe someday draw a triangle. 🔺

𝗪𝗵𝘆 𝗮𝗺 𝗜 𝗱𝗼𝗶𝗻𝗴 𝘁𝗵𝗶𝘀?

Some people pick peaceful hobbies like gardening or meditation.
I chose Vulkan — the API equivalent of assembling IKEA furniture blindfolded while the parts are on fire. 🔥

𝗪𝗵𝗮𝘁’𝘀 𝗱𝗼𝗻𝗲 𝘀𝗼 𝗳𝗮𝗿:

✅ Cross-platform windowing (Windows + Linux)
✅ Event/Input system (it remembers what keys you hit — unlike me)
✅ Custom memory allocator with 19 debug tags (because Vulkan leaks don’t cry, they scream)
✅ Double-buffered input state (so "just pressed" vs "held down" is a thing)
✅ Vulkan device + queue management (the GPU now acknowledges my existence)

So yeah — massive progress, despite spending half my time Googling "why is Vulkan like this?"

𝗪𝗵𝗮𝘁’𝘀 𝗻𝗲𝘅𝘁?

Swap chain
Render pass
Graphics pipeline
Sacrificing a GPU to the graphics gods for first triangle luck 🙏

Target: Render a triangle next week
Reality: Probably arguing with VkResult codes at 3AM

𝗪𝗵𝘆 𝗶𝘁'𝘀 𝘄𝗼𝗿𝘁𝗵 𝗶𝘁:

I now know exactly what happens when a game runs.
Every pointer. Every buffer. Every terrifying Vulkan struct.
No magic — just pain beautiful low-level control.

If you’ve ever wondered what game engines actually do, or you just enjoy watching someone suffer in real-time, stick around — I'll be posting weekly updates.

The code is public. Judge me here 👇

https://lnkd.in/dSGphPhs

Thank you for reading through!

hashtag#GameDev hashtag#Vulkan hashtag#CProgramming hashtag#SystemsProgramming hashtag#GraphicsProgramming