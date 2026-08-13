class WebUIWidgetSurfacePlot extends WebUIWidgetCanvas
{
    static template()
    {
        return [
            {'name': "SURFACE PLOT", 'control':'header'},
            {'name':'title', 'default':"Surface Plot", 'type':'string', 'control':'textedit'},
            {'name':'source', 'default':"", 'type':'source', 'control':'textedit'},
            {'name':'x_source', 'default':"", 'type':'source', 'control':'textedit'},
            {'name':'y_source', 'default':"", 'type':'source', 'control':'textedit'},
            {'name':'color_source', 'default':"", 'type':'source', 'control':'textedit'},
            {'name':'mask_source', 'default':"", 'type':'source', 'control':'textedit'},

            {'name': "SURFACE", 'control':'header'},
            {'name':'display_mode', 'default':"surface-wireframe", 'type':'string', 'control':'menu', 'options':"surface,wireframe,surface-wireframe,points"},
            {'name':'stride', 'default':1, 'type':'int', 'control':'textedit'},
            {'name':'height_scale', 'default':4, 'type':'float', 'control':'textedit'},
            {'name':'surface_opacity', 'default':1, 'min':0, 'max':1, 'type':'float', 'control':'slider'},
            {'name':'surface_roughness', 'default':0.72, 'min':0, 'max':1, 'type':'float', 'control':'slider'},
            {'name':'wireframe_color', 'default':'#263238', 'type':'string', 'control':'textedit'},
            {'name':'point_size', 'default':3, 'type':'float', 'control':'textedit'},

            {'name': "HEIGHT RANGE", 'control':'header'},
            {'name':'auto_height_range', 'default':"yes", 'type':'bool', 'control':'checkbox'},
            {'name':'z_min', 'default':-1, 'type':'float', 'control':'textedit'},
            {'name':'z_max', 'default':1, 'type':'float', 'control':'textedit'},

            {'name': "COLOR", 'control':'header'},
            {'name':'color_mode', 'default':"height", 'type':'string', 'control':'menu', 'options':"height,source,fixed"},
            {'name':'surface_color', 'default':'#4da3d9', 'type':'string', 'control':'textedit'},
            {'name':'color_map', 'default':"spectrum", 'type':'string', 'control':'menu', 'options':"gray,fire,spectrum,custom"},
            {'name':'color_map_colors', 'default':'', 'type':'string', 'control':'textedit'},
            {'name':'auto_color_range', 'default':"yes", 'type':'bool', 'control':'checkbox'},
            {'name':'value_min', 'default':-1, 'type':'float', 'control':'textedit'},
            {'name':'value_max', 'default':1, 'type':'float', 'control':'textedit'},
            {'name':'show_color_legend', 'default':"yes", 'type':'bool', 'control':'checkbox'},
            {'name':'color_legend_width', 'default':14, 'type':'int', 'control':'textedit'},
            {'name':'color_legend_ticks', 'default':5, 'type':'int', 'control':'textedit'},
            {'name':'color_legend_decimals', 'default':2, 'type':'int', 'control':'textedit'},

            {'name': "COORDINATES", 'control':'header'},
            {'name':'x_min', 'default':0, 'type':'float', 'control':'textedit'},
            {'name':'x_max', 'default':1, 'type':'float', 'control':'textedit'},
            {'name':'y_min', 'default':0, 'type':'float', 'control':'textedit'},
            {'name':'y_max', 'default':1, 'type':'float', 'control':'textedit'},
            {'name':'flip_x', 'default':"no", 'type':'bool', 'control':'checkbox'},
            {'name':'flip_y', 'default':"no", 'type':'bool', 'control':'checkbox'},
            {'name':'flip_z', 'default':"no", 'type':'bool', 'control':'checkbox'},

            {'name': "CAMERA", 'control':'header'},
            {'name':'projection', 'default':"perspective", 'type':'string', 'control':'menu', 'options':"perspective,orthographic"},
            {'name':'camera_azimuth', 'default':45, 'type':'float', 'control':'textedit'},
            {'name':'camera_elevation', 'default':32, 'type':'float', 'control':'textedit'},
            {'name':'camera_distance', 'default':16, 'type':'float', 'control':'textedit'},
            {'name':'field_of_view', 'default':45, 'type':'float', 'control':'textedit'},
            {'name':'orthographic_size', 'default':13, 'type':'float', 'control':'textedit'},
            {'name':'auto_rotate', 'default':"no", 'type':'bool', 'control':'checkbox'},
            {'name':'auto_rotate_speed', 'default':0.5, 'type':'float', 'control':'textedit'},

            {'name': "SCENE", 'control':'header'},
            {'name':'scene_background', 'default':'#f5f7f8', 'type':'string', 'control':'textedit'},
            {'name':'show_grid', 'default':"yes", 'type':'bool', 'control':'checkbox'},
            {'name':'grid_divisions', 'default':10, 'type':'int', 'control':'textedit'},
            {'name':'show_axes', 'default':"yes", 'type':'bool', 'control':'checkbox'},
            {'name':'show_bounding_box', 'default':"yes", 'type':'bool', 'control':'checkbox'},
            {'name':'x_label', 'default':'X', 'type':'string', 'control':'textedit'},
            {'name':'y_label', 'default':'Y', 'type':'string', 'control':'textedit'},
            {'name':'z_label', 'default':'Z', 'type':'string', 'control':'textedit'},
            {'name':'axis_color', 'default':'#263238', 'type':'string', 'control':'textedit'},
        ];
    }

    static html()
    {
        return `<canvas class="surface-overlay"></canvas><canvas class="surface-webgl"></canvas>`;
    }

    init()
    {
        super.init();
        this.overlayCanvas = this.canvasElement;
        this.webglCanvas = this.querySelector('.surface-webgl');
        this.style.position = "relative";
        this.overlayCanvas.style.position = "absolute";
        this.overlayCanvas.style.inset = "0";
        this.overlayCanvas.style.zIndex = "2";
        this.overlayCanvas.style.pointerEvents = "none";
        this.webglCanvas.style.position = "absolute";
        this.webglCanvas.style.left = "0";
        this.webglCanvas.style.top = "0";
        this.webglCanvas.style.zIndex = "1";
        this.webglCanvas.style.touchAction = "none";

        this.scene = new THREE.Scene();
        this.renderer = new THREE.WebGLRenderer({canvas:this.webglCanvas, antialias:true});
        this.renderer.setPixelRatio(window.devicePixelRatio || 1);
        this.renderer.outputEncoding = THREE.sRGBEncoding;

        this.surfaceRoot = new THREE.Group();
        this.decorationRoot = new THREE.Group();
        this.scene.add(this.surfaceRoot);
        this.scene.add(this.decorationRoot);
        this.scene.add(new THREE.HemisphereLight(0xffffff, 0x66717a, 1.05));
        this.directionalLight = new THREE.DirectionalLight(0xffffff, 0.72);
        this.directionalLight.position.set(-8, 12, 10);
        this.scene.add(this.directionalLight);

        this.camera = null;
        this.controls = null;
        this.geometry = null;
        this.pointGeometry = null;
        this.surfaceMesh = null;
        this.wireframeMesh = null;
        this.points = null;
        this.positionArray = null;
        this.colorArray = null;
        this.validity = null;
        this.rowIndices = [];
        this.columnIndices = [];
        this.topologyKey = "";
        this.validityInitialized = false;
        this.sceneWidth = 0;
        this.sceneHeight = 0;
        this.cameraKey = "";
        this.cameraViewKey = "";
        this.decorationKey = "";
        this.materialKey = "";
        this.colorMapKey = "";
        this.colorMapColors = [];
        this.colorMapRGB = [];
        this.currentHeightRange = {min:-1, max:1};
        this.currentColorRange = {min:-1, max:1};
        this.animationFrame = null;
        this.lastDataError = "";

        this.addManagedListener(this.webglCanvas, "wheel", (event) => {
            if(typeof main !== "undefined" && main.edit_mode)
                return;
            if(event.cancelable)
                event.preventDefault();
            event.stopPropagation();
        }, {passive:false});

        this.configureCamera(true);
        this.refreshMaterials();
        this.refreshDecorations();
        this.resizeRenderer();
        this.animate();
    }

    disconnectedCallback()
    {
        super.disconnectedCallback();
        if(this.animationFrame !== null)
            cancelAnimationFrame(this.animationFrame);
        this.animationFrame = null;
        if(this.controls && typeof this.controls.dispose === "function")
            this.controls.dispose();
        this.disposeSurface();
        this.clearGroup(this.decorationRoot);
        if(this.renderer)
            this.renderer.dispose();
    }

    clearGroup(group)
    {
        if(!group)
            return;
        while(group.children.length > 0)
        {
            const child = group.children[group.children.length - 1];
            group.remove(child);
            child.traverse((node) => {
                if(node.geometry)
                    node.geometry.dispose();
                if(node.material)
                {
                    const materials = Array.isArray(node.material) ? node.material : [node.material];
                    for(const material of materials)
                    {
                        if(material.map)
                            material.map.dispose();
                        material.dispose();
                    }
                }
            });
        }
    }

    disposeSurface()
    {
        if(this.surfaceMesh)
            this.surfaceRoot.remove(this.surfaceMesh);
        if(this.wireframeMesh)
            this.surfaceRoot.remove(this.wireframeMesh);
        if(this.points)
            this.surfaceRoot.remove(this.points);
        if(this.geometry)
            this.geometry.dispose();
        if(this.pointGeometry)
            this.pointGeometry.dispose();
        const materials = [this.surfaceMaterial, this.wireframeMaterial, this.pointMaterial];
        for(const material of materials)
            if(material)
                material.dispose();
        this.geometry = null;
        this.pointGeometry = null;
        this.surfaceMesh = null;
        this.wireframeMesh = null;
        this.points = null;
        this.surfaceMaterial = null;
        this.wireframeMaterial = null;
        this.pointMaterial = null;
        this.materialKey = "";
    }

    animate()
    {
        this.animationFrame = requestAnimationFrame(() => this.animate());
        if(this.controls)
        {
            this.controls.enabled = !(typeof main !== "undefined" && main.edit_mode);
            this.controls.autoRotate = this.toBool(this.parameters.auto_rotate) && this.controls.enabled;
            this.controls.autoRotateSpeed = Number(this.parameters.auto_rotate_speed) || 0.5;
            this.controls.update();
        }
        if(this.renderer && this.camera)
            this.renderer.render(this.scene, this.camera);
    }

    configureCamera(force=false)
    {
        const projection = this.parameters.projection === "orthographic" ? "orthographic" : "perspective";
        const fov = Math.max(5, Math.min(150, Number(this.parameters.field_of_view) || 45));
        const orthographicSize = Math.max(1, Number(this.parameters.orthographic_size) || 13);
        const key = `${projection}:${fov}:${orthographicSize}`;
        if(!force && key === this.cameraKey)
            return;

        const oldPosition = this.camera ? this.camera.position.clone() : null;
        const oldTarget = this.controls ? this.controls.target.clone() : null;
        if(this.controls)
            this.controls.dispose();
        this.cameraKey = key;
        if(projection === "orthographic")
            this.camera = new THREE.OrthographicCamera(-orthographicSize / 2, orthographicSize / 2, orthographicSize / 2, -orthographicSize / 2, 0.01, 1000);
        else
            this.camera = new THREE.PerspectiveCamera(fov, 1, 0.01, 1000);

        this.controls = new THREE.OrbitControls(this.camera, this.webglCanvas);
        this.controls.enableDamping = true;
        this.controls.dampingFactor = 0.08;
        this.controls.enablePan = true;
        this.controls.minDistance = 2;
        this.controls.maxDistance = 100;
        if(oldPosition && oldTarget)
        {
            this.camera.position.copy(oldPosition);
            this.controls.target.copy(oldTarget);
        }
        else
            this.resetCamera();
        this.controls.update();
        this.cameraViewKey = [
            this.parameters.camera_azimuth,
            this.parameters.camera_elevation,
            this.parameters.camera_distance,
            this.parameters.height_scale,
        ].join(':');
        this.sceneWidth = 0;
        this.sceneHeight = 0;
    }

    resetCamera()
    {
        if(!this.camera || !this.controls)
            return;
        const azimuth = (Number(this.parameters.camera_azimuth) || 45) * Math.PI / 180;
        const elevation = (Number(this.parameters.camera_elevation) || 32) * Math.PI / 180;
        const distance = Math.max(2, Number(this.parameters.camera_distance) || 16);
        const targetY = Math.max(0, Number(this.parameters.height_scale) || 4) * 0.35;
        this.controls.target.set(0, targetY, 0);
        this.camera.position.set(
            distance * Math.cos(elevation) * Math.sin(azimuth),
            targetY + distance * Math.sin(elevation),
            distance * Math.cos(elevation) * Math.cos(azimuth)
        );
        this.camera.lookAt(this.controls.target);
    }

    updateConfiguredCameraView()
    {
        const key = [
            this.parameters.camera_azimuth,
            this.parameters.camera_elevation,
            this.parameters.camera_distance,
            this.parameters.height_scale,
        ].join(':');
        if(key === this.cameraViewKey)
            return;
        this.cameraViewKey = key;
        this.resetCamera();
        this.controls.update();
    }

    resizeRenderer()
    {
        super.updateFrame();
        this.configureCamera();
        this.updateConfiguredCameraView();
        const width = Math.max(1, this.offsetWidth);
        const height = Math.max(1, this.offsetHeight);
        const legendSpace = this.hasVerticalColorLegend() && this.parameters.color_mode !== "fixed" ? this.getVerticalColorLegendSpace() : 0;
        const renderWidth = Math.max(1, width - legendSpace);
        if(renderWidth === this.sceneWidth && height === this.sceneHeight)
            return;

        this.sceneWidth = renderWidth;
        this.sceneHeight = height;
        this.renderer.setSize(renderWidth, height, false);
        this.webglCanvas.style.width = `${renderWidth}px`;
        this.webglCanvas.style.height = `${height}px`;
        const aspect = renderWidth / height;
        if(this.camera.isOrthographicCamera)
        {
            const size = Math.max(1, Number(this.parameters.orthographic_size) || 13);
            this.camera.left = -size * aspect / 2;
            this.camera.right = size * aspect / 2;
            this.camera.top = size / 2;
            this.camera.bottom = -size / 2;
        }
        else
            this.camera.aspect = aspect;
        this.camera.updateProjectionMatrix();
    }

    isMatrix(data)
    {
        if(!Array.isArray(data) || data.length === 0 || !Array.isArray(data[0]) || data[0].length === 0)
            return false;
        const columns = data[0].length;
        for(const row of data)
            if(!Array.isArray(row) || row.length !== columns)
                return false;
        return true;
    }

    compatibleMatrix(data, rows, columns)
    {
        return this.isMatrix(data) && data.length === rows && data[0].length === columns;
    }

    vectorData(name, length)
    {
        const source = this.getSource(name, null);
        if(Array.isArray(source) && source.length === length && !Array.isArray(source[0]))
            return source;
        if(Array.isArray(source) && source.length === 1 && Array.isArray(source[0]) && source[0].length === length)
            return source[0];
        return null;
    }

    sampleIndices(length, stride)
    {
        const indices = [];
        for(let index = 0; index < length; index += stride)
            indices.push(index);
        if(indices[indices.length - 1] !== length - 1)
            indices.push(length - 1);
        return indices;
    }

    ensureTopology(rows, columns)
    {
        const stride = Math.max(1, Math.trunc(Number(this.parameters.stride) || 1));
        const key = `${rows}:${columns}:${stride}`;
        if(key === this.topologyKey)
            return;

        this.disposeSurface();
        this.topologyKey = key;
        this.rowIndices = this.sampleIndices(rows, stride);
        this.columnIndices = this.sampleIndices(columns, stride);
        const vertexCount = this.rowIndices.length * this.columnIndices.length;
        this.positionArray = new Float32Array(vertexCount * 3);
        this.colorArray = new Float32Array(vertexCount * 3);
        this.validity = new Uint8Array(vertexCount);
        this.validityInitialized = false;

        this.geometry = new THREE.BufferGeometry();
        const positions = new THREE.BufferAttribute(this.positionArray, 3);
        const colors = new THREE.BufferAttribute(this.colorArray, 3);
        positions.setUsage(THREE.DynamicDrawUsage);
        colors.setUsage(THREE.DynamicDrawUsage);
        this.geometry.setAttribute('position', positions);
        this.geometry.setAttribute('color', colors);
        this.pointGeometry = new THREE.BufferGeometry();
        this.pointGeometry.setAttribute('position', positions);
        this.pointGeometry.setAttribute('color', colors);

        this.surfaceMaterial = new THREE.MeshStandardMaterial({
            vertexColors:true,
            side:THREE.DoubleSide,
            roughness:0.72,
            metalness:0,
        });
        this.wireframeMaterial = new THREE.MeshBasicMaterial({wireframe:true, transparent:true});
        this.pointMaterial = new THREE.PointsMaterial({vertexColors:true, size:3, sizeAttenuation:false});
        this.surfaceMesh = new THREE.Mesh(this.geometry, this.surfaceMaterial);
        this.wireframeMesh = new THREE.Mesh(this.geometry, this.wireframeMaterial);
        this.points = new THREE.Points(this.pointGeometry, this.pointMaterial);
        this.surfaceRoot.add(this.surfaceMesh);
        this.surfaceRoot.add(this.wireframeMesh);
        this.surfaceRoot.add(this.points);
        this.materialKey = "";
        this.refreshMaterials();
    }

    finiteRange(data, mask, fallbackMin, fallbackMax)
    {
        let minimum = Infinity;
        let maximum = -Infinity;
        for(let row = 0; row < data.length; row++)
            for(let column = 0; column < data[row].length; column++)
            {
                if(mask && !(Number(mask[row][column]) > 0))
                    continue;
                const value = Number(data[row][column]);
                if(!Number.isFinite(value))
                    continue;
                minimum = Math.min(minimum, value);
                maximum = Math.max(maximum, value);
            }
        if(!Number.isFinite(minimum) || !Number.isFinite(maximum))
            return {min:fallbackMin, max:fallbackMax};
        if(minimum === maximum)
        {
            const padding = Math.max(0.5, Math.abs(minimum) * 0.05);
            minimum -= padding;
            maximum += padding;
        }
        return {min:minimum, max:maximum};
    }

    configuredRange(auto, data, mask, minimumName, maximumName, fallbackMin, fallbackMax)
    {
        if(this.toBool(auto))
            return this.finiteRange(data, mask, fallbackMin, fallbackMax);
        let minimum = Number(this.parameters[minimumName]);
        let maximum = Number(this.parameters[maximumName]);
        if(!Number.isFinite(minimum))
            minimum = fallbackMin;
        if(!Number.isFinite(maximum))
            maximum = fallbackMax;
        if(minimum === maximum)
            maximum = minimum + 1;
        if(minimum > maximum)
            [minimum, maximum] = [maximum, minimum];
        return {min:minimum, max:maximum};
    }

    refreshColorMap()
    {
        const key = `${this.parameters.color_map}:${this.parameters.color_map_colors}:${this.parameters.surface_color}`;
        if(key === this.colorMapKey)
            return;
        this.colorMapKey = key;
        this.colorMapColors = this.getColorMap(this.parameters.color_map, this.parameters.color_map_colors);
        this.colorMapRGB = this.colorMapColors.map((color) => {
            const parsed = new THREE.Color(color);
            return [parsed.r, parsed.g, parsed.b];
        });
        this.fixedColor = new THREE.Color(this.parameters.surface_color || "#4da3d9");
    }

    writeColor(offset, value, range)
    {
        if(this.parameters.color_mode === "fixed")
        {
            this.colorArray[offset] = this.fixedColor.r;
            this.colorArray[offset + 1] = this.fixedColor.g;
            this.colorArray[offset + 2] = this.fixedColor.b;
            return;
        }
        const fraction = range.max !== range.min ? (value - range.min) / (range.max - range.min) : 0;
        const index = Math.max(0, Math.min(this.colorMapRGB.length - 1, Math.floor(fraction * this.colorMapRGB.length)));
        const color = this.colorMapRGB[index] || [0, 0, 0];
        this.colorArray[offset] = color[0];
        this.colorArray[offset + 1] = color[1];
        this.colorArray[offset + 2] = color[2];
    }

    updateIndexIfNeeded(changed)
    {
        if(!changed && this.validityInitialized)
            return;
        const indices = [];
        const pointIndices = [];
        const rows = this.rowIndices.length;
        const columns = this.columnIndices.length;
        for(let row = 0; row < rows - 1; row++)
            for(let column = 0; column < columns - 1; column++)
            {
                const a = row * columns + column;
                const b = a + 1;
                const c = a + columns;
                const d = c + 1;
                if(this.validity[a] && this.validity[b] && this.validity[c])
                    indices.push(a, c, b);
                if(this.validity[b] && this.validity[c] && this.validity[d])
                    indices.push(b, c, d);
            }
        for(let vertex = 0; vertex < this.validity.length; vertex++)
            if(this.validity[vertex])
                pointIndices.push(vertex);
        this.geometry.setIndex(indices);
        this.pointGeometry.setIndex(pointIndices);
        this.validityInitialized = true;
    }

    updateGeometry(heightData, colorData, maskData)
    {
        const rows = heightData.length;
        const columns = heightData[0].length;
        this.ensureTopology(rows, columns);
        this.refreshColorMap();
        const xData = this.vectorData('x_source', columns);
        const yData = this.vectorData('y_source', rows);
        const heightRange = this.configuredRange(this.parameters.auto_height_range, heightData, maskData, 'z_min', 'z_max', -1, 1);
        const colorValues = this.parameters.color_mode === "source" && colorData ? colorData : heightData;
        const colorRange = this.parameters.color_mode === "fixed" ? heightRange :
            this.configuredRange(this.parameters.auto_color_range, colorValues, maskData, 'value_min', 'value_max', heightRange.min, heightRange.max);
        this.currentHeightRange = heightRange;
        this.currentColorRange = colorRange;

        const xMinimum = Number(this.parameters.x_min);
        const xMaximum = Number(this.parameters.x_max);
        const yMinimum = Number(this.parameters.y_min);
        const yMaximum = Number(this.parameters.y_max);
        const xRange = Number.isFinite(xMaximum - xMinimum) && xMaximum !== xMinimum ? xMaximum - xMinimum : 1;
        const yRange = Number.isFinite(yMaximum - yMinimum) && yMaximum !== yMinimum ? yMaximum - yMinimum : 1;
        const heightScale = Math.max(0.01, Number(this.parameters.height_scale) || 4);
        let validityChanged = !this.validityInitialized;
        let vertex = 0;
        for(const sourceRow of this.rowIndices)
            for(const sourceColumn of this.columnIndices)
            {
                const height = Number(heightData[sourceRow][sourceColumn]);
                const maskValid = !maskData || Number(maskData[sourceRow][sourceColumn]) > 0;
                const valid = Number.isFinite(height) && maskValid;
                if(this.validity[vertex] !== (valid ? 1 : 0))
                    validityChanged = true;
                this.validity[vertex] = valid ? 1 : 0;
                const offset = vertex * 3;
                let xFraction = xData ? (Number(xData[sourceColumn]) - xMinimum) / xRange : sourceColumn / Math.max(1, columns - 1);
                let yFraction = yData ? (Number(yData[sourceRow]) - yMinimum) / yRange : sourceRow / Math.max(1, rows - 1);
                if(this.toBool(this.parameters.flip_x))
                    xFraction = 1 - xFraction;
                if(this.toBool(this.parameters.flip_y))
                    yFraction = 1 - yFraction;
                let heightFraction = valid ? (height - heightRange.min) / (heightRange.max - heightRange.min) : 0;
                if(this.toBool(this.parameters.flip_z))
                    heightFraction = 1 - heightFraction;
                this.positionArray[offset] = 10 * (xFraction - 0.5);
                this.positionArray[offset + 1] = heightScale * heightFraction;
                this.positionArray[offset + 2] = 10 * (yFraction - 0.5);
                const colorValue = colorValues && colorValues[sourceRow] ? Number(colorValues[sourceRow][sourceColumn]) : height;
                this.writeColor(offset, Number.isFinite(colorValue) ? colorValue : height, colorRange);
                vertex++;
            }

        this.updateIndexIfNeeded(validityChanged);
        this.geometry.attributes.position.needsUpdate = true;
        this.geometry.attributes.color.needsUpdate = true;
        this.refreshMaterials();
        if(this.surfaceMesh.visible)
            this.geometry.computeVertexNormals();
        this.refreshDecorations();
    }

    refreshMaterials()
    {
        if(!this.surfaceMaterial)
            return;
        const mode = this.parameters.display_mode || "surface-wireframe";
        const key = [
            mode,
            this.parameters.surface_opacity,
            this.parameters.surface_roughness,
            this.parameters.wireframe_color,
            this.parameters.point_size,
        ].join(':');
        if(key === this.materialKey)
            return;
        this.materialKey = key;
        this.surfaceMesh.visible = mode === "surface" || mode === "surface-wireframe";
        this.wireframeMesh.visible = mode === "wireframe" || mode === "surface-wireframe";
        this.points.visible = mode === "points";
        const opacity = Math.max(0, Math.min(1, Number(this.parameters.surface_opacity)));
        this.surfaceMaterial.opacity = Number.isFinite(opacity) ? opacity : 1;
        this.surfaceMaterial.transparent = this.surfaceMaterial.opacity < 1;
        this.surfaceMaterial.depthWrite = this.surfaceMaterial.opacity >= 1;
        this.surfaceMaterial.roughness = Math.max(0, Math.min(1, Number(this.parameters.surface_roughness) || 0));
        this.surfaceMaterial.needsUpdate = true;
        this.wireframeMaterial.color.set(this.parameters.wireframe_color || "#263238");
        this.pointMaterial.size = Math.max(1, Number(this.parameters.point_size) || 3);
    }

    createTextSprite(text, color)
    {
        const canvas = document.createElement('canvas');
        canvas.width = 256;
        canvas.height = 96;
        const context = canvas.getContext('2d');
        context.font = "bold 44px sans-serif";
        context.textAlign = "center";
        context.textBaseline = "middle";
        context.lineWidth = 8;
        context.strokeStyle = "rgba(255,255,255,0.9)";
        context.strokeText(String(text), 128, 48);
        context.fillStyle = color;
        context.fillText(String(text), 128, 48);
        const texture = new THREE.CanvasTexture(canvas);
        texture.minFilter = THREE.LinearFilter;
        const material = new THREE.SpriteMaterial({map:texture, transparent:true, depthTest:false});
        const sprite = new THREE.Sprite(material);
        sprite.scale.set(1.8, 0.68, 1);
        return sprite;
    }

    refreshDecorations()
    {
        if(!this.decorationRoot)
            return;
        const heightScale = Math.max(0.01, Number(this.parameters.height_scale) || 4);
        const key = [
            heightScale,
            this.parameters.show_grid,
            this.parameters.grid_divisions,
            this.parameters.show_axes,
            this.parameters.show_bounding_box,
            this.parameters.x_label,
            this.parameters.y_label,
            this.parameters.z_label,
            this.parameters.axis_color,
            this.parameters.scene_background,
        ].join(':');
        if(key === this.decorationKey)
            return;
        this.decorationKey = key;
        this.clearGroup(this.decorationRoot);
        this.scene.background = new THREE.Color(this.parameters.scene_background || "#f5f7f8");
        const color = this.parameters.axis_color || "#263238";
        if(this.toBool(this.parameters.show_grid))
        {
            const divisions = Math.max(1, Math.trunc(Number(this.parameters.grid_divisions) || 10));
            const grid = new THREE.GridHelper(10, divisions, color, color);
            grid.material.opacity = 0.22;
            grid.material.transparent = true;
            this.decorationRoot.add(grid);
        }
        if(this.toBool(this.parameters.show_bounding_box))
        {
            const boxGeometry = new THREE.BoxGeometry(10, heightScale, 10);
            const edges = new THREE.EdgesGeometry(boxGeometry);
            boxGeometry.dispose();
            const box = new THREE.LineSegments(edges, new THREE.LineBasicMaterial({color, transparent:true, opacity:0.45}));
            box.position.y = heightScale / 2;
            this.decorationRoot.add(box);
        }
        if(this.toBool(this.parameters.show_axes))
        {
            const axes = new THREE.AxesHelper(Math.max(5, heightScale));
            axes.position.set(-5, 0.01, -5);
            this.decorationRoot.add(axes);
            const labels = [
                [this.parameters.x_label || "X", 5.6, 0.15, -5, "#c62828"],
                [this.parameters.y_label || "Y", -5, 0.15, 5.6, "#1565c0"],
                [this.parameters.z_label || "Z", -5, heightScale + 0.5, -5, "#2e7d32"],
            ];
            for(const [text, x, y, z, labelColor] of labels)
            {
                const sprite = this.createTextSprite(text, labelColor);
                sprite.position.set(x, y, z);
                this.decorationRoot.add(sprite);
            }
        }
    }

    drawOverlay(message="")
    {
        this.clearCanvas(0, 0);
        const width = this.format.width;
        const height = this.format.height;
        if(this.hasVerticalColorLegend() && this.parameters.color_mode !== "fixed")
        {
            const legendWidth = this.getVerticalColorLegendSpace();
            const x = width - legendWidth + 8;
            this.drawVerticalColorLegend(x, 22, Math.max(1, height - 44), this.currentColorRange.min, this.currentColorRange.max, this.colorMapColors);
        }
        if(message)
        {
            this.canvas.save();
            this.canvas.fillStyle = this.format.axis_color || "#263238";
            this.canvas.font = "13px sans-serif";
            this.canvas.textAlign = "center";
            this.canvas.textBaseline = "middle";
            this.canvas.fillText(message, width / 2, height / 2);
            this.canvas.restore();
        }
    }

    update()
    {
        this.resizeRenderer();
        const heightData = this.getSource('source', null);
        if(!this.isMatrix(heightData))
        {
            this.lastDataError = "Surface Plot requires a rank-2 source";
            this.surfaceRoot.visible = false;
            this.drawOverlay(this.lastDataError);
            return;
        }
        const rows = heightData.length;
        const columns = heightData[0].length;
        const requestedColor = this.getSource('color_source', null);
        const requestedMask = this.getSource('mask_source', null);
        const colorData = this.compatibleMatrix(requestedColor, rows, columns) ? requestedColor : null;
        const maskData = this.compatibleMatrix(requestedMask, rows, columns) ? requestedMask : null;
        this.lastDataError = "";
        this.surfaceRoot.visible = true;
        this.updateGeometry(heightData, colorData, maskData);
        this.drawOverlay();
    }
}


webui_widgets.add('webui-widget-surface-plot', WebUIWidgetSurfacePlot);
