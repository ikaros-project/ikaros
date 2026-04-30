class WebUIWidgetJoystick extends WebUIWidgetControl {
    static template() {
        return [
            { name: "JOYSTICK", control: "header" },
            { name: "title", default: "Joystick", type: "string", control: "textedit" },

            { name: "CONTROL", control: "header" },
            { name: "parameter_x", default: "", type: "source", control: "textedit" },
            { name: "parameter_y", default: "", type: "source", control: "textedit" },
            { name: "select_x", default: 0, type: "int", control: "textedit" },
            { name: "select_y", default: "", type: "string", control: "textedit" },

            { name: "BEHAVIOR", control: "header" },
            { name: "return_to_center", default: true, type: "bool", control: "checkbox" },
            { name: "range", default: "bipolar", type: "string", control: "menu", options: "bipolar,unipolar" }
        ];
    }

    static html() {
        return `
            <div class="joystick">
                <div class="joystick-pad">
                    <div class="joystick-axis joystick-axis-x"></div>
                    <div class="joystick-axis joystick-axis-y"></div>
                    <div class="joystick-thumb"></div>
                </div>
            </div>
        `;
    }

    disconnectedCallback() {
        if (typeof super.disconnectedCallback === "function") {
            super.disconnectedCallback();
        }
        this._unbindDocumentDragHandlers();
    }

    _getPad() {
        return this.querySelector(".joystick-pad");
    }

    _getThumb() {
        return this.querySelector(".joystick-thumb");
    }

    _isUnipolar() {
        return String(this.parameters.range || "").toLowerCase() === "unipolar";
    }

    _centerPosition() {
        return { x: 0.5, y: 0.5 };
    }

    _clamp01(value) {
        return Math.max(0, Math.min(1, Number(value)));
    }

    _normalizedToValue(value) {
        const clamped = this._clamp01(value);
        if (this._isUnipolar()) {
            return clamped;
        }
        return clamped * 2 - 1;
    }

    _valueToNormalized(value) {
        const number = Number(value);
        if (!Number.isFinite(number)) {
            return 0.5;
        }
        if (this._isUnipolar()) {
            return this._clamp01(number);
        }
        return this._clamp01((number + 1) / 2);
    }

    _formatValue(value) {
        return Number(value).toFixed(4).replace(/\.?0+$/, "");
    }

    _getSelectY() {
        if (this.parameters.select_y === undefined || this.parameters.select_y === null) {
            return "";
        }
        return this.parameters.select_y;
    }

    _sendAxisValue(parameter, value) {
        if (!parameter) {
            return;
        }

        const x = Math.trunc(Number(this.parameters.select_x || 0));
        const y = this._getSelectY();
        if (y === "") {
            this.send_control_change(parameter, this._formatValue(value), x);
            return;
        }
        this.send_control_change(parameter, this._formatValue(value), x, Math.trunc(Number(y)));
    }

    _sendPosition(position) {
        this._sendAxisValue(this.parameters.parameter_x, this._normalizedToValue(position.x));
        this._sendAxisValue(this.parameters.parameter_y, this._normalizedToValue(position.y));
    }

    _setThumbPosition(position) {
        const thumb = this._getThumb();
        const pad = this._getPad();
        if (!thumb || !pad) {
            return;
        }

        const x = this._clamp01(position.x);
        const y = this._clamp01(position.y);
        const maxLeft = Math.max(0, pad.clientWidth - thumb.offsetWidth);
        const maxTop = Math.max(0, pad.clientHeight - thumb.offsetHeight);
        this.position = { x, y };
        thumb.style.left = `${x * maxLeft}px`;
        thumb.style.top = `${y * maxTop}px`;
    }

    _positionFromPointer(event) {
        const pad = this._getPad();
        if (!pad) {
            return this.position || this._centerPosition();
        }

        const rect = pad.getBoundingClientRect();
        const thumb = this._getThumb();
        const thumbWidth = thumb?.offsetWidth || 0;
        const thumbHeight = thumb?.offsetHeight || 0;
        const width = Math.max(1, rect.width - thumbWidth);
        const height = Math.max(1, rect.height - thumbHeight);

        return {
            x: this._clamp01((event.clientX - rect.left - thumbWidth / 2) / width),
            y: this._clamp01((event.clientY - rect.top - thumbHeight / 2) / height)
        };
    }

    _dragTo(event) {
        const position = this._positionFromPointer(event);
        this._setThumbPosition(position);
        this._sendPosition(position);
    }

    _unbindDocumentDragHandlers() {
        if (this._dragMoveHandler) {
            document.removeEventListener("mousemove", this._dragMoveHandler, true);
            document.removeEventListener("touchmove", this._dragMoveHandler, true);
            this._dragMoveHandler = null;
        }
        if (this._dragEndHandler) {
            document.removeEventListener("mouseup", this._dragEndHandler, true);
            document.removeEventListener("touchend", this._dragEndHandler, true);
            document.removeEventListener("touchcancel", this._dragEndHandler, true);
            this._dragEndHandler = null;
        }
    }

    _eventPoint(event) {
        if (event.touches && event.touches.length > 0) {
            return event.touches[0];
        }
        if (event.changedTouches && event.changedTouches.length > 0) {
            return event.changedTouches[0];
        }
        return event;
    }

    _startDrag(event) {
        if (main.edit_mode) {
            return;
        }

        event.preventDefault();
        event.stopPropagation();
        this.is_active = true;

        this._dragTo(this._eventPoint(event));

        this._dragMoveHandler = (moveEvent) => {
            moveEvent.preventDefault();
            this._dragTo(this._eventPoint(moveEvent));
        };

        this._dragEndHandler = (endEvent) => {
            endEvent.preventDefault();
            endEvent.stopPropagation();
            this._unbindDocumentDragHandlers();

            if (this.toBool(this.parameters.return_to_center)) {
                const center = this._centerPosition();
                this._setThumbPosition(center);
                this._sendPosition(center);
            }

            this.is_active = false;
        };

        document.addEventListener("mousemove", this._dragMoveHandler, true);
        document.addEventListener("mouseup", this._dragEndHandler, true);
        document.addEventListener("touchmove", this._dragMoveHandler, { capture: true, passive: false });
        document.addEventListener("touchend", this._dragEndHandler, true);
        document.addEventListener("touchcancel", this._dragEndHandler, true);
    }

    _readSourceValue(parameterName) {
        if (!this.parameters[parameterName]) {
            return undefined;
        }

        let data = this.getSource(parameterName);
        if (data === undefined || data === null) {
            return undefined;
        }

        const y = this._getSelectY();
        const x = Math.trunc(Number(this.parameters.select_x || 0));

        if (Array.isArray(data) && y !== "") {
            return data[Math.trunc(Number(y))]?.[x];
        }

        if (Array.isArray(data) && Array.isArray(data[0])) {
            return data[0]?.[x];
        }

        if (Array.isArray(data)) {
            return data[x];
        }

        return data;
    }

    updateAll() {
        super.updateAll();

        const pad = this._getPad();
        if (pad && !this._joystickHandlersBound) {
            pad.addEventListener("mousedown", (event) => this._startDrag(event), false);
            pad.addEventListener("touchstart", (event) => this._startDrag(event), { passive: false });
            this._joystickHandlersBound = true;
        }

        if (!this.position) {
            this._setThumbPosition(this._centerPosition());
        }
    }

    update() {
        if (this.is_active) {
            return;
        }

        try {
            const hasXParameter = !!this.parameters.parameter_x;
            const hasYParameter = !!this.parameters.parameter_y;
            const x = this._readSourceValue("parameter_x");
            const y = this._readSourceValue("parameter_y");

            this._setThumbPosition({
                x: hasXParameter && x !== undefined ? this._valueToNormalized(x) : 0.5,
                y: hasYParameter && y !== undefined ? this._valueToNormalized(y) : 0.5
            });
        }
        catch (err) {
        }
    }
}

webui_widgets.add("webui-widget-joystick", WebUIWidgetJoystick);
