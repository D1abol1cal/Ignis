𝐓𝐡𝐞 𝐈𝐠𝐧𝐢𝐬 𝐄𝐧𝐠𝐢𝐧𝐞 𝐣𝐮𝐬𝐭 𝐫𝐞𝐧𝐝𝐞𝐫𝐞𝐝 𝐢𝐭𝐬 𝐟𝐢𝐫𝐬𝐭 𝐟𝐫𝐚𝐦𝐞 𝐮𝐬𝐢𝐧𝐠 𝐕𝐮𝐥𝐤𝐚𝐧!

𝐖𝐞𝐞𝐤 7: 𝐂𝐨𝐦𝐩𝐥𝐞𝐭𝐢𝐨𝐧 𝐨𝐟 𝐭𝐡𝐞 𝐕𝐮𝐥𝐤𝐚𝐧 𝐑𝐞𝐧𝐝𝐞𝐫𝐢𝐧𝐠 𝐏𝐢𝐩𝐞𝐥𝐢𝐧𝐠.
After laying groundwork for weeks, the core Vulkan rendering pipeline is now fully operational. The engine can clear the screen and run a complete render loop which is the foundation for all future graphics work.

𝐖𝐡𝐚𝐭 𝐈 𝐢𝐦𝐩𝐥𝐞𝐦𝐞𝐧𝐭𝐞𝐝 𝐭𝐡𝐢𝐬 𝐰𝐞𝐞𝐤:
• 𝐑𝐞𝐧𝐝𝐞𝐫 𝐏𝐚𝐬𝐬 𝐀𝐫𝐜𝐡𝐢𝐭𝐞𝐜𝐭𝐮𝐫𝐞:
I built the Vulkan render pass system, which defines how GPU rendering operations are structured. This includes configuring color and depth attachments and managing sub pass dependencies, effectively describing what happens to framebuffer contents before, during, and after rendering.

• 𝐂𝐨𝐦𝐦𝐚𝐧𝐝 𝐁𝐮𝐟𝐟𝐞𝐫 𝐒𝐲𝐬𝐭𝐞𝐦:
I implemented the command buffer infrastructure from scratch. In Vulkan, every GPU operation such as binding pipelines or issuing draw calls must be recorded into these buffers. This included command pool management, allocation, and recording systems.

• 𝐅𝐫𝐚𝐦𝐞𝐛𝐮𝐟𝐟𝐞𝐫 𝐌𝐚𝐧𝐚𝐠𝐞𝐦𝐞𝐧𝐭:
I created a framebuffer system linking image views and render passes. Each swap chain image now has a dedicated framebuffer that’s recreated on window resize with proper cleanup.

• 𝐒𝐲𝐧𝐜𝐡𝐫𝐨𝐧𝐢𝐳𝐚𝐭𝐢𝐨𝐧 𝐏𝐫𝐢𝐦𝐢𝐭𝐢𝐯𝐞𝐬:
I implemented fences and semaphores, Vulkan’s explicit synchronization tools. Fences coordinate CPU ↔ GPU operations, while semaphores handle GPU ↔ GPU dependencies such as “wait before rendering” or “wait before presentation.”

• 𝐑𝐞𝐧𝐝𝐞𝐫 𝐋𝐨𝐨𝐩 𝐈𝐧𝐭𝐞𝐠𝐫𝐚𝐭𝐢𝐨𝐧:
Finally, I tied everything together into a functional render loop:
• Acquires swap chain images
• Records command buffers with render pass operations
• Submits work to GPU queues with correct synchronization
• Presents rendered frames to the screen
• Handles window resizing by recreating swap chain-dependent resources

𝐓𝐞𝐜𝐡𝐧𝐢𝐜𝐚𝐥 𝐂𝐡𝐚𝐥𝐥𝐞𝐧𝐠𝐞𝐬:
The trickiest part was synchronization as Vulkan offers zero automation so you must manage every wait and signal manually. Getting fences and semaphores correctly chained was crucial to avoid validation errors and race conditions.

Handling window resizing was another challenge, ensuring all GPU operations are finished before recreating swap chains, framebuffers, and attachments took careful sequencing.

𝐍𝐞𝐱𝐭 𝐮𝐩:
Creating the graphics pipeline with vertex and fragment shaders to bring visible geometry to life.

The source code is public, so feel free to have a look. 

https://github.com/D1abol1cal/Ignis

If anybody wants to contribute, then feel free to message me!

Here’s a quick look at the current state of Ignis: A frame rendered through Vulkan.

#GameDev #Vulkan #CProgramming #SystemsProgramming #GraphicsProgramming