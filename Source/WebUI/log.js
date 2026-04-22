const log =
{
    height_cookie: 'log_height',
    resize_threshold: 12,
    min_height: 40,
    max_height_margin: 80,
    resize_active: false,

    isOnResizeBorder(evt)
    {
        if(!log.view)
            return false;
        const r = log.view.getBoundingClientRect();
        const y = evt.clientY - r.top;
        return y >= 0 && y <= log.resize_threshold;
    },

    updateResizeCursor(evt)
    {
    },

    startResize(evt)
    {
        if(evt.button !== 0)
            return;
        const fromHandle = !!evt.target.closest('.log-resize-handle');
        if(!fromHandle && !log.isOnResizeBorder(evt))
            return;

        evt.preventDefault();
        log.resize_active = true;
        log.resize_start_y = evt.clientY;
        log.resize_start_height = log.view.getBoundingClientRect().height;
        document.body.classList.add('log-resizing');
        document.addEventListener('mousemove', log.onResizeDrag, true);
        document.addEventListener('mouseup', log.stopResize, true);
    },

    onResizeDrag(evt)
    {
        if(!log.resize_active || !log.view)
            return;
        const deltaY = evt.clientY - log.resize_start_y;
        const maxHeight = Math.max(log.min_height, window.innerHeight - log.max_height_margin);
        const newHeight = Math.max(log.min_height, Math.min(maxHeight, log.resize_start_height - deltaY));
        log.view.style.flex = `0 0 ${newHeight}px`;
        log.view.style.height = `${newHeight}px`;
        evt.preventDefault();
    },

    stopResize()
    {
        if(!log.resize_active)
            return;
        log.resize_active = false;
        document.removeEventListener('mousemove', log.onResizeDrag, true);
        document.removeEventListener('mouseup', log.stopResize, true);
        document.body.classList.remove('log-resizing');
        if(log.view)
        {
            const h = Math.round(log.view.getBoundingClientRect().height);
            if(Number.isFinite(h) && h >= log.min_height)
                setCookie(log.height_cookie, String(h));
        }
    },

    getMessagesElement()
    {
        if(!log.view)
            log.view = document.querySelector('footer');
        if(!log.view)
            return null;

        if(!log.messagesElement || !log.view.contains(log.messagesElement))
            log.messagesElement = log.view.querySelector('.log-messages');

        if(!log.messagesElement)
        {
            log.messagesElement = document.createElement('div');
            log.messagesElement.className = 'log-messages';
            log.view.appendChild(log.messagesElement);
        }
        return log.messagesElement;
    },

    init()
    {
        log.view = document.querySelector('footer');
        log.button = document.getElementById('log_toggle_button');
        log.messagesElement = log.getMessagesElement();
        if(log.view)
        {
            log.resizeHandle = log.view.querySelector('.log-resize-handle');
            if(!log.resizeHandle)
            {
                log.resizeHandle = document.createElement('div');
                log.resizeHandle.className = 'log-resize-handle';
                log.view.prepend(log.resizeHandle);
            }
            const savedHeight = parseInt(getCookie(log.height_cookie), 10);
            if(Number.isFinite(savedHeight) && savedHeight >= log.min_height)
            {
                const maxHeight = Math.max(log.min_height, window.innerHeight - log.max_height_margin);
                const clampedHeight = Math.min(savedHeight, maxHeight);
                log.view.style.flex = `0 0 ${clampedHeight}px`;
                log.view.style.height = `${clampedHeight}px`;
            }
            log.resizeHandle.addEventListener('mousedown', log.startResize, true);
        }
    },

    setAlert(state)
    {
        if(!log.button)
            return;
        log.button.classList.toggle('log-alert', !!state);
    },

    toggleLog()
    {
        log.setAlert(false);
        const s = window.getComputedStyle(log.view, null);
        if(s.display === 'none')
            log.view.style.display = 'block';
        else
            log.view.style.display = 'none';
    },

    close()
    {
        log.setAlert(false);
        if(log.view)
            log.view.style.display = 'none';
    },

    showView(fromAlert=false)
    {
        log.view.style.display = 'block';
        if(fromAlert)
            log.setAlert(true);
    },

    clear()
    {
        const logElement = log.getMessagesElement();
        if(logElement)
            logElement.innerHTML = '';
        else if(log.view)
            log.view.querySelectorAll('p').forEach((e) => e.remove());
        log.setAlert(false);
    },

    print(message)
    {
        let logElement = log.getMessagesElement() || log.view || document.querySelector('.log');
        if(!logElement)
            return;
        let p = document.createElement('p');
        p.className = 'warning';
        p.appendChild(document.createTextNode(message));
        logElement.appendChild(p);
    }
};
