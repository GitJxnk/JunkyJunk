// ==UserScript==
// @name         V
// @namespace    http://tampermonkey.net/
// @version      1.0.0
// @description  xd
// @author       MA
// @match        https://www.kogama.com/*
// @icon         https://www.google.com/s2/favicons?sz=64&domain=kogama.com
// @grant        none
// ==/UserScript==

const style = document.createElement('style');
style.type = 'text/css';

const css = `
  * {
    transition: all 0.2s cubic-bezier(0.4, 0, 0.2, 1) !important;
}

  body, .css-1995t1d, .css-e5yc1l {
    background: linear-gradient(135deg, #0f0c29, #302b63, #24243e) !important;
    font-family: 'Poppins', -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif !important;
}

  ::-webkit-scrollbar {
    width: 8px;
    height: 8px;
}
  ::-webkit-scrollbar-track {
    background: rgba(255,255,255,0.05) !important;
    border-radius: 10px !important;
}
  ::-webkit-scrollbar-thumb {
    background: linear-gradient(135deg, #667eea, #764ba2) !important;
    border-radius: 10px !important;
}
  ::-webkit-scrollbar-thumb:hover {
    background: linear-gradient(135deg, #764ba2, #667eea) !important;
}

  .css-p9g1ic, ._1q4mD ._1sUGu ._290sk, ._1q4mD ._1sUGu ._1u05O {
    background: rgba(10, 10, 20, 0.8) !important;
    backdrop-filter: blur(20px) !important;
    border-bottom: 1px solid rgba(255,255,255,0.1) !important;
    box-shadow: 0 4px 30px rgba(0,0,0,0.3) !important;
}

  img[alt="KoGaMa"], .logo img {
    filter: drop-shadow(0 0 15px rgba(102,126,234,0.6)) !important;
    animation: float 3s ease-in-out infinite !important;
}

  @keyframes float {
    0%, 100% { transform: translateY(0px); }
    50% { transform: translateY(-5px); }
}

  button, .btn, [class*="button"], ._2UG5l, .c_7Wt, ._355_H, .css-ry78up, .css-fvu3zm, .css-yrsbmg {
    background: linear-gradient(135deg, #667eea, #764ba2) !important;
    border: none !important;
    border-radius: 50px !important;
    padding: 10px 25px !important;
    color: white !important;
    font-weight: 600 !important;
    letter-spacing: 0.5px !important;
    box-shadow: 0 4px 15px rgba(102,126,234,0.4) !important;
    position: relative !important;
    overflow: hidden !important;
    cursor: pointer !important;
}

  button::before, .btn::before, ._2UG5l::before, .c_7Wt::before, ._355_H::before, .css-ry78up::before, .css-fvu3zm::before, .css-yrsbmg::before {
    content: '' !important;
    position: absolute !important;
    top: 50% !important;
    left: 50% !important;
    width: 0 !important;
    height: 0 !important;
    background: rgba(255,255,255,0.2) !important;
    border-radius: 50% !important;
    transform: translate(-50%, -50%) !important;
    transition: width 0.6s, height 0.6s !important;
}

  button:hover::before, .btn:hover::before, ._2UG5l:hover::before, .c_7Wt:hover::before, ._355_H:hover::before, .css-ry78up:hover::before, .css-fvu3zm:hover::before, .css-yrsbmg:hover::before {
    width: 300px !important;
    height: 300px !important;
}

  button:hover, .btn:hover, ._2UG5l:hover, .c_7Wt:hover, ._355_H:hover, .css-ry78up:hover, .css-fvu3zm:hover, .css-yrsbmg:hover {
    transform: translateY(-2px) scale(1.05) !important;
    box-shadow: 0 8px 25px rgba(102,126,234,0.6) !important;
}

  .css-wog98n, .css-l2vsff, .css-o4yc28, ._3TORb, .css-l5ts0, .css-13qd5rb, .css-1y0281p, .css-z05bui, ._1Yt8Y, ._2ybM3 ._2Jzqh ._37JQp ._1pBXO, .css-1j12w2b {
    background: rgba(20, 20, 40, 0.6) !important;
    backdrop-filter: blur(10px) !important;
    border-radius: 20px !important;
    border: 1px solid rgba(255,255,255,0.1) !important;
    box-shadow: 0 8px 32px rgba(0,0,0,0.2) !important;
    transition: all 0.3s !important;
}

  .css-wog98n:hover, .css-l2vsff:hover, .css-o4yc28:hover, ._3TORb:hover, .css-l5ts0:hover, .css-13qd5rb:hover, .css-1y0281p:hover, .css-z05bui:hover, ._1Yt8Y:hover, ._2ybM3:hover ._2Jzqh:hover ._37JQp:hover ._1pBXO:hover, .css-1j12w2b:hover {
    transform: translateY(-5px) !important;
    border-color: rgba(102,126,234,0.5) !important;
    box-shadow: 0 15px 45px rgba(102,126,234,0.3) !important;
}

  a, .link {
    color: #a78bfa !important;
    text-decoration: none !important;
    position: relative !important;
}

  a::after, .link::after {
    content: '' !important;
    position: absolute !important;
    bottom: -2px !important;
    left: 0 !important;
    width: 0% !important;
    height: 2px !important;
    background: linear-gradient(90deg, #667eea, #764ba2) !important;
    transition: width 0.3s !important;
}

  a:hover::after, .link:hover::after {
    width: 100% !important;
}

  .gmqKr a {
    position: initial !important;
    display: inline-block !important;
    padding: 5px 10px !important;
    border-radius: 8px !important;
}

  .gmqKr a:hover {
    background: rgba(102,126,234,0.3) !important;
    transform: translateY(-2px) !important;
}

  h1, h2, h3, h4, h5, h6 {
    background: linear-gradient(135deg, #fff, #a78bfa) !important;
    -webkit-background-clip: text !important;
    background-clip: text !important;
    color: transparent !important;
    font-weight: 700 !important;
}

  input, textarea, select {
    background: rgba(255,255,255,0.05) !important;
    border: 1px solid rgba(255,255,255,0.1) !important;
    border-radius: 12px !important;
    color: white !important;
    padding: 12px !important;
    transition: all 0.3s !important;
}

  input:focus, textarea:focus, select:focus {
    outline: none !important;
    border-color: #667eea !important;
    box-shadow: 0 0 20px rgba(102,126,234,0.3) !important;
    background: rgba(255,255,255,0.1) !important;
}

  footer {
    background: rgba(0,0,0,0.5) !important;
    backdrop-filter: blur(10px) !important;
    border-top: 1px solid rgba(255,255,255,0.05) !important;
}

  @media (max-width: 768px) {
    body {
      padding: 10px !important;
    }
    button, .btn {
      padding: 8px 16px !important;
      font-size: 14px !important;
  }
}

  .glow-text {
    text-shadow: 0 0 10px rgba(102,126,234,0.5) !important;
}
`;

style.innerHTML = css;
document.head.appendChild(style);

(function () {
    const gifURL = "https://cdn3.emoji.gg/emojis/5988-pixelbongocat.gif";

    function replaceLogo() {
        const logo = document.querySelector('img[alt="KoGaMa"]');
        if (logo) {
            logo.removeAttribute('srcset');
            logo.src = gifURL;
            logo.style.transform = "scale(1.1)";
        }
    }

    window.onload = replaceLogo;

    const observer = new MutationObserver(() => {
        replaceLogo();
    });

    observer.observe(document.body, {
        childList: true,
        subtree: true,
    });

    window.addEventListener("resize", replaceLogo);

})();
