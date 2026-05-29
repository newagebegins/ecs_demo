let gl;
let ext;
let lastTimeMs;
let frameTimeElement = document.getElementById("frame-time");

function main()
{
    const memory = new WebAssembly.Memory({ initial: 2 });

    Promise.all([
        WebAssembly.instantiateStreaming(fetch("game.wasm"), { env: { memory } }),
        fetch('atlas.bin').then(response => response.arrayBuffer()),
    ]).then(([wasmModule, atlasBuffer]) => {
        const [
            atlasWidth,
            atlasHeight,
            bitmapInfosSize,
            atlasPixelsSize,
            atlasInfosOffset,
            atlasPixelsOffset,
        ] = Array.from(new Uint32Array(atlasBuffer, 0, 6).values());
        
        const pixels = new Uint8Array(atlasBuffer, atlasPixelsOffset, atlasPixelsSize);

        // Memory map (starting at __heap_base):
        // struct game_memory
        // struct game_input
        // permanentStorage (permanentStorageSize)
        // bitmap_info (bitmapInfosSize)
        // renderList (renderListSize)

        const gameMemoryOffset = wasmModule.instance.exports.__heap_base;
        const gameMemoryLength = 6;
        const gameMemorySize = gameMemoryLength*Uint32Array.BYTES_PER_ELEMENT;

        const gameInputOffset = gameMemoryOffset + gameMemorySize;
        const gameInputLength = 11;
        const gameInputSize = gameInputLength*Uint32Array.BYTES_PER_ELEMENT;

        let permanentStorageOffset = gameInputOffset + gameInputSize;
        let permanentStorageSize = 4096;

        let bitmapInfosOffset = permanentStorageOffset + permanentStorageSize;

        let renderListOffset = bitmapInfosOffset + bitmapInfosSize;
        let renderListSize = 8192;

        const renderListUsedIndex = 4;
        const renderListBitmapCountIndex = 5;

        const gameMemory = new Uint32Array(memory.buffer, gameMemoryOffset, gameMemoryLength);
        gameMemory.set([
            permanentStorageOffset,
            permanentStorageSize,
            renderListOffset,
            renderListSize,
            0, // RenderListUsed
            0, // RenderListBitmapCount
        ]);

        let infosSource = new Uint8Array(atlasBuffer, atlasInfosOffset, bitmapInfosSize);
        const memoryUint8 = new Uint8Array(memory.buffer);
        memoryUint8.set(infosSource, bitmapInfosOffset);

        let screenWidth = 1024;
        let screenHeight = 768;
        let screenScale = 1;

        let canvas = document.getElementById("game-canvas");
        canvas.width = screenWidth*screenScale;
        canvas.height = screenHeight*screenScale;

        gl = canvas.getContext("webgl");
        if(!gl)
        {
            alert("ERROR: WebGL is not supported in your browser");
            return;
        }

        ext = gl.getExtension('ANGLE_instanced_arrays');
        if(!ext)
        {
            alert('ANGLE_instanced_arrays is not supported');
            return;
        }

        const vertexShaderSource = `
            attribute vec2 position;

            // Instanced attributes:
            attribute vec2 scale;
            attribute vec2 offset;
            attribute vec2 uvScale;
            attribute vec2 uvOffset;
            attribute vec4 color;

            uniform mat4 screenToClip;
            varying vec2 uv;
            varying vec4 vColor;

            void main()
            {
                uv = uvScale*position + uvOffset;
                vColor = color;
                gl_Position = screenToClip*vec4(scale*position + offset, 0.0, 1.0);
            }
        `;

        const fragmentShaderSource = `
            precision highp float;

            varying vec2 uv;
            varying vec4 vColor;

            uniform sampler2D image;

            void main()
            {
                gl_FragColor = texture2D(image, uv) * vColor;
            }
        `;

        let vertexShader = gl.createShader(gl.VERTEX_SHADER);
        let fragmentShader = gl.createShader(gl.FRAGMENT_SHADER);

        gl.shaderSource(vertexShader, vertexShaderSource);
        gl.shaderSource(fragmentShader, fragmentShaderSource);

        gl.compileShader(vertexShader);
        gl.compileShader(fragmentShader);

        let program = gl.createProgram();
        gl.attachShader(program, vertexShader);
        gl.attachShader(program, fragmentShader);
        gl.linkProgram(program);

        if(gl.getProgramParameter(program, gl.LINK_STATUS))
        {
            gl.deleteShader(vertexShader);
            gl.deleteShader(fragmentShader);

            let vertexBuffer = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, vertexBuffer);
            let quadVertices = new Float32Array([
                1, 1,
                0, 1,
                1, 0,
                0, 0,
            ]);
            gl.bufferData(gl.ARRAY_BUFFER, quadVertices, gl.STATIC_DRAW);
            let positionLocation = gl.getAttribLocation(program, "position");
            gl.enableVertexAttribArray(positionLocation);
            gl.vertexAttribPointer(positionLocation, 2, gl.FLOAT, false, 0, 0);

            let instanceBuffer = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, instanceBuffer);
            gl.bufferData(gl.ARRAY_BUFFER, renderListSize, gl.DYNAMIC_DRAW);

            let stride = 12*4;

            let scaleLocation = gl.getAttribLocation(program, "scale");
            gl.enableVertexAttribArray(scaleLocation);
            gl.vertexAttribPointer(scaleLocation, 2, gl.FLOAT, false, stride, 0);
            ext.vertexAttribDivisorANGLE(scaleLocation, 1);

            let offsetLocation = gl.getAttribLocation(program, "offset");
            gl.enableVertexAttribArray(offsetLocation);
            gl.vertexAttribPointer(offsetLocation, 2, gl.FLOAT, false, stride, 2*4);
            ext.vertexAttribDivisorANGLE(offsetLocation, 1);

            let uvScaleLocation = gl.getAttribLocation(program, "uvScale");
            gl.enableVertexAttribArray(uvScaleLocation);
            gl.vertexAttribPointer(uvScaleLocation, 2, gl.FLOAT, false, stride, 4*4);
            ext.vertexAttribDivisorANGLE(uvScaleLocation, 1);

            let uvOffsetLocation = gl.getAttribLocation(program, "uvOffset");
            gl.enableVertexAttribArray(uvOffsetLocation);
            gl.vertexAttribPointer(uvOffsetLocation, 2, gl.FLOAT, false, stride, 6*4);
            ext.vertexAttribDivisorANGLE(uvOffsetLocation, 1);

            let colorLocation = gl.getAttribLocation(program, "color");
            gl.enableVertexAttribArray(colorLocation);
            gl.vertexAttribPointer(colorLocation, 4, gl.FLOAT, false, stride, 8*4);
            ext.vertexAttribDivisorANGLE(colorLocation, 1);

            let screenToClipLocation = gl.getUniformLocation(program, "screenToClip");
            console.assert(screenToClipLocation !== null);

            gl.useProgram(program);

            let screenToClip = [
                2/screenWidth,0,0,0,
                0,2/screenHeight,0,0,
                0,0,1,0,
                -1,-1,0,1,
            ];
            gl.uniformMatrix4fv(screenToClipLocation, false, screenToClip);

            let texture = gl.createTexture();
            gl.bindTexture(gl.TEXTURE_2D, texture);
            gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, atlasWidth, atlasHeight, 0, gl.RGBA, gl.UNSIGNED_BYTE, pixels);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);

            gl.enable(gl.BLEND);
            gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);

            const gameInputView = new DataView(memory.buffer, gameInputOffset, gameInputSize);

            let gameInputAt = 0
            const dtOffset = gameInputAt;
            gameInputAt += 4;
            const leftIsDownOffset = gameInputAt;
            gameInputAt += 4;
            const leftJustWentDownOffset = gameInputAt;
            gameInputAt += 4;
            const rightIsDownOffset = gameInputAt;
            gameInputAt += 4;
            const rightJustWentDownOffset = gameInputAt;
            gameInputAt += 4;
            const upIsDownOffset = gameInputAt;
            gameInputAt += 4;
            const upJustWentDownOffset = gameInputAt;
            gameInputAt += 4;
            const downIsDownOffset = gameInputAt;
            gameInputAt += 4;
            const downJustWentDownOffset = gameInputAt;
            gameInputAt += 4;
            const actionIsDownOffset = gameInputAt;
            gameInputAt += 4;
            const actionJustWentDownOffset = gameInputAt;
            gameInputAt += 4;

            const buttons = {
                'KeyA': {
                    isDownOffset: leftIsDownOffset,
                    justWentDownOffset: leftJustWentDownOffset,
                },
                'KeyD': {
                    isDownOffset: rightIsDownOffset,
                    justWentDownOffset: rightJustWentDownOffset,
                },
                'KeyW': {
                    isDownOffset: upIsDownOffset,
                    justWentDownOffset: upJustWentDownOffset,
                },
                'KeyS': {
                    isDownOffset: downIsDownOffset,
                    justWentDownOffset: downJustWentDownOffset,
                },
                'Space': {
                    isDownOffset: actionIsDownOffset,
                    justWentDownOffset: actionJustWentDownOffset,
                },
            };

            window.addEventListener('keydown', (e) => {
                if (Object.hasOwn(buttons, e.code)) {
                    e.preventDefault();
                    if (gameInputView.getUint32(buttons[e.code].isDownOffset, true) == 0) {
                        gameInputView.setUint32(buttons[e.code].justWentDownOffset, 1, true);
                    }
                    gameInputView.setUint32(buttons[e.code].isDownOffset, 1, true);
                }
            });

            window.addEventListener('keyup', (e) => {
                if (Object.hasOwn(buttons, e.code)) {
                    e.preventDefault();
                    gameInputView.setUint32(buttons[e.code].isDownOffset, 0, true);
                }
            });

            lastTimeMs = performance.now();
            mainLoop(lastTimeMs);

            function mainLoop(currentTimeMs)
            {
                requestAnimationFrame(mainLoop);

                let deltaMs = currentTimeMs - lastTimeMs;
                lastTimeMs = currentTimeMs;

                frameTimeElement.textContent = deltaMs.toFixed(2);

                let maxDeltaMs = 17;
                if(deltaMs > maxDeltaMs)
                {
                    deltaMs = maxDeltaMs;
                }

                let dt = deltaMs / 1000;

                gameInputView.setFloat32(dtOffset, dt, true);

                wasmModule.instance.exports.GameUpdateAndRender(gameMemoryOffset, gameInputOffset, bitmapInfosOffset);

                Object.values(buttons).forEach(({ justWentDownOffset }) => {
                    gameInputView.setUint32(justWentDownOffset, 0, true);
                });

                const renderListUsed = gameMemory.at(renderListUsedIndex);
                const view = new DataView(memory.buffer, renderListOffset, renderListUsed);
                const instanceCount = gameMemory.at(renderListBitmapCountIndex);
                const floatsPerInstance = 12; // see struct render_entry_bitmap
                const instanceData = new Float32Array(instanceCount * floatsPerInstance);
                let at = 0;
                let instanceDataAt = 0;

                while(at < renderListUsed)
                {
                    const id = view.getUint32(at, true);
                    at += 4;
                    switch(id)
                    {
                        // RenderEntry_Clear,
                        case 0: {
                            const r = view.getFloat32(at, true);
                            at += 4;
                            const g = view.getFloat32(at, true);
                            at += 4;
                            const b = view.getFloat32(at, true);
                            at += 4;
                            const a = view.getFloat32(at, true);
                            at += 4;
                            gl.clearColor(r, g, b, a);
                            gl.clear(gl.COLOR_BUFFER_BIT);
                            break;
                        }
                        // RenderEntry_Bitmap,
                        case 1: {
                            instanceData.set(new Float32Array(memory.buffer, renderListOffset + at, floatsPerInstance), instanceDataAt);
                            at += floatsPerInstance * Float32Array.BYTES_PER_ELEMENT;
                            instanceDataAt += floatsPerInstance;
                            break;
                        }
                        // RenderEntry_Rect,
                        case 2: {
                            // NOTE(slava): We don't support rects in the web layer
                            break;
                        }
                        default: {
                            console.error(`Unknown render entry ID: ${id}`);
                            break;
                        }
                    }
                }
                console.assert(at == renderListUsed);

                gl.bindBuffer(gl.ARRAY_BUFFER, instanceBuffer);
                gl.bufferSubData(gl.ARRAY_BUFFER, 0, instanceData);

                const vertsPerInstance = 4;
                ext.drawArraysInstancedANGLE(gl.TRIANGLE_STRIP, 0, vertsPerInstance, instanceCount);
            }
        }
        else
        {
            console.error("Link failed: ", gl.getProgramInfoLog(program));
            console.error("Vertex shader log: ", gl.getShaderInfoLog(vertexShader));
            console.error("Fragment shader log: ", gl.getShaderInfoLog(fragmentShader));
        }
    });
}

main();
