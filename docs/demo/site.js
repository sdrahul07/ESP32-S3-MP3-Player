(() => {
  'use strict';
  const $ = selector => document.querySelector(selector);
  const frame = $('#preview-frame');
  let themes = [], current = 0, view = 'interactive', screen = 'player';
  function scale() {
    const value = Number($('#scale').value);
    document.documentElement.style.setProperty('--screen-width', `${320 * value}px`);
    frame.contentWindow?.postMessage({type:'prototype-scale', scale:value}, location.origin);
  }
  function renderImage() {
    const item = themes[current];
    $('#firmware-image').src = screen === 'player' ? `previews/now-playing/${item.preview}` : `previews/menus/${String(item.number).padStart(2,'0')}-${screen}.png`;
    $('#firmware-image').alt = `${item.label}: ${screen}, saved firmware renderer output`;
  }
  function renderList() {
    const query = $('#theme-search').value.toLowerCase();
    const list = $('#theme-list'); list.replaceChildren();
    themes.forEach((item, index) => {
      if (!`${item.label} ${item.group}`.toLowerCase().includes(query)) return;
      const button = document.createElement('button');
      button.type = 'button'; button.setAttribute('aria-pressed', String(index === current));
      const number = document.createElement('span'); number.textContent = String(item.number).padStart(2,'0');
      button.append(number, document.createTextNode(item.label));
      button.onclick = () => { location.hash = item.id; };
      list.append(button);
    });
    if (!list.childElementCount) { const p = document.createElement('p'); p.textContent = 'No matching themes.'; list.append(p); }
  }
  function selectTheme() {
    const id = decodeURIComponent(location.hash.slice(1));
    const found = themes.findIndex(item => item.id === id);
    current = found < 0 ? 0 : found;
    const item = themes[current];
    $('#theme-title').textContent = item.label;
    $('#theme-number').textContent = `${String(item.number).padStart(2,'0')} / 20`;
    $('#control-hint').textContent = item.controls;
    document.title = `${item.label} · ES32-S3 MP3 Player`;
    frame.title = `${item.label} interactive concept`;
    // Unload inactive previews: only the chosen concept keeps an animation loop.
    frame.src = `demo/themes/${item.id}.html`;
    renderList(); renderImage(); $('#status').textContent = '';
  }
  function setView(next) {
    view = next;
    $('#interactive-view').hidden = view !== 'interactive';
    $('#firmware-view').hidden = view !== 'firmware';
    document.querySelectorAll('[data-view]').forEach(button => button.setAttribute('aria-pressed', String(button.dataset.view === view)));
    // Removing the iframe while viewing static renders stops concept timers.
    if(view === 'firmware') frame.src = 'about:blank';
    else frame.src = `demo/themes/${themes[current].id}.html`;
  }
  window.addEventListener('message', event => { if(event.source === frame.contentWindow && event.origin === location.origin && event.data?.type === 'prototype-ready') scale(); });
  frame.addEventListener('load', scale);
  $('#scale').onchange = scale;
  $('#theme-search').oninput = renderList;
  $('#previous-theme').onclick = () => { location.hash = themes[(current + themes.length - 1) % themes.length].id; };
  $('#next-theme').onclick = () => { location.hash = themes[(current + 1) % themes.length].id; };
  window.addEventListener('hashchange', () => { selectTheme(); if(view === 'firmware') frame.src = 'about:blank'; });
  document.querySelectorAll('[data-view]').forEach(button => button.onclick = () => setView(button.dataset.view));
  document.querySelectorAll('[data-screen]').forEach(button => button.onclick = () => {
    screen = button.dataset.screen;
    document.querySelectorAll('[data-screen]').forEach(other => other.setAttribute('aria-pressed', String(other === button)));
    renderImage();
  });
  $('#copy-link').onclick = async () => {
    const link = new URL(location.href); link.hash = themes[current].id;
    try { await navigator.clipboard.writeText(link.href); $('#status').textContent = 'Theme link copied.'; }
    catch { $('#status').textContent = 'Copy this page’s address to share the selected theme.'; }
  };
  window.addEventListener('keydown', event => {
    if(view !== 'interactive' || event.target.matches('button,input,select,textarea,a')) return;
    const keys = {' ':'play',ArrowUp:'up',ArrowDown:'down',ArrowLeft:'previous',ArrowRight:'next',Enter:'center',Escape:'menu'};
    if(keys[event.key]) { event.preventDefault(); frame.contentWindow?.postMessage({type:'prototype-command',action:keys[event.key]}, location.origin); }
  });
  fetch('demo/themes.json').then(response => { if(!response.ok) throw new Error('Theme list unavailable'); return response.json(); }).then(data => {themes = data; selectTheme();}).catch(() => {
    $('#theme-list').textContent = 'Could not load themes. Serve docs/ over HTTP (see README), or open an individual preview file.';
  });
})();
