let ping_rate = 5000;
let unreachable_count = 0;
let unreachable = false;

function back_pressed() {
  console.log("back pressed");
  history.back()
}

function forward_pressed() {
  console.log("forward pressed");
  history.forward()
}

function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

function reload() {
  window.location.reload(true);
}

function popup(title, msg) {  
  console.log("popu pressed");
  
  let new_div = document.createElement("div");
  new_div.classList.add("popup");
  
  new_div.innerHTML =
  ` 
  <div class=\"popup-outer\" onclick="popup_outer_clicked(this.parentNode)"></div>
  <div class=\"popup-inner\">
    <div class="popup-cancel-row" onclick="popup_close_clicked(this.parentNode.parentNode)"><b class="popup-cancel">X</b></div>
    <div class=\"popup-title\"><b>` + title + `</b></div>
    <div class=\"popup-msg\">` + msg + `</div>
  </div>
  `

  let parent = document.getElementById('content');
  parent.appendChild(new_div);
}

function popup_outer_clicked(element) {
  console.log("outer clicked");
    
  element.parentNode.removeChild(element);
}

function popup_close_clicked(element) {
  console.log("outer clicked");
  
  element.parentNode.removeChild(element);
}

function popup_backend_unreachable() {
 let title = "Backend Unreachable";
 let msg = "The backend may have crashed. Restart Shog Studio";

  let new_div = document.createElement("div");
  new_div.classList.add("popup");
  new_div.classList.add("popup-unreachable");
  
  new_div.innerHTML =
  ` 
  <div class="popup-outer"></div>
  <div class="popup-inner">
    <svg version="1.2" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 50 50" class="popup-img">
    	<defs>
    		<image  width="50" height="50" id="img1" href="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADIAAAAyCAMAAAAp4XiDAAAAAXNSR0IB2cksfwAAAWhQTFRFjUoJklMVlVcbml8mm2Eoml4kk1QXjksLlVcat41l1Lqh6NrN69/U8Ofe8ejg7+Xc6dzP3827vZZwnmYvmFsgvZdx4dC/+vbz/////Pv56t3Rx6aFomw3kVAStoti38y6+/n36+DVvpdyl1ofxaOD/fz72sWwmVwi59nLsIJVllgcza+T8uriqHVDxqWE3su5tYpg+PTw1r6m0LSa0bac3sq4yquNklIUs4Za9vHsoGgyuI5m9/Luq3pLkVAR5dbHqndHnGIqr4BS1byj2cOt9O7o7ePYrX1Ot4xjq3lJs4Zb49PD5tfJp3RCn2cx1Lui9/PvkE4Pw5992sSv6t7T59nM/v38upFp7ODW28aynWQsl1oe8+3mnmUuyamK3ci1uZBpn2Ywv5l1xKGAvJVu/v79mFwh28Wx7eLXoms1soRYkVETpG876t7SsYNWoWkz8urj9e/q2cOupXA9nWMrsIFU9e/pupJqIOTYIAAAAgtJREFUeJxjZCAZMI5qwSYMAT+J1cLBiABvidAiAlKIYtsT/FpkGRl/oAqBrLyLRwuj8p8fGA7hZbz/G6cWdcYPGBpA4LvUPaSAQNaihc2zYCDyAclDSFrYlLDbAbZH7jIWLbKCr3DqYGAQvwn3JEKLwXNkR0oyMt7mQuLz8p/B0KLF/RjZVLlL+owPkAUUGY+ha7FmvIOsQvUwg90tFKfJXP6JpsX+JooCjQMMDBIoIiKi+1G1cEj8IKCFUXMfqhZRZhR5LFoYtPeianG5QlCL7m4ULYxcvAS1MLz+i6xFxOgSYS36O5G1sHBxoUpj06J8lGQtBjuQtTBzcRPWYrgdWQsbFweqtNE2BlZhNC1Kx5C1sLucRZU2Aebfk6hCFozrkbUwBB1jIASs1zKgaLF6/RlFnsOE8c5zFBE7xpWoWhgiDqAocGRkvIya5VwYF6Npidv9H1kBpvfd7x9C0yJjg2KNxwKGxO3IArwcsNyPyLxJh5F9oyvHuO0fEt/33VoGDC1pjBsZcIMAxumYWhhYuThx6tD9uw/ORi5Vsm5fxlQMAV6cU7FqYU9djN0eXdl5SDzUYlyX6RtqjIKAnTjj36lIfLTKgkOH9T6aDl557YkoAhhVktUvm60Im3x5GBl70VRgVnzsuYznjBdB7DRWOfz4MboCHNVr1NsvDH+ZxNdjkxtqTYWB1wIAhDl0M0QMVMIAAAAASUVORK5CYII="/>
    	</defs>
    	<use id="Background" href="#img1" x="0" y="0"/>
    </svg>
    <div class="popup-title"><b>` + title + `</b></div>
    <div class="popup-msg">` + msg + `</div>
  </div>
  `

  let parent = document.getElementById('content');
  parent.appendChild(new_div);
}

function close_popup_backend_unreachable() {
  let element = document.querySelector(".popup-unreachable");
    
  element.parentNode.removeChild(element);
}

function close_toast(element) {
  element.classList.add('fade-out');

  element.addEventListener('animationend', function() {
    element.parentNode.removeChild(element);
  }, { once: true });
}

function toast_big(title, msg) {
  let new_div = document.createElement("div");
  new_div.classList.add("toast-big");
  
  new_div.innerHTML =
  ` 
  <div class="toast-title-big"><b>` + title + `</b> <b class="toast-close" onclick="close_toast(this.parentNode.parentNode)">X</b></div>
  <div class="toast-msg">` + msg + `</div>
  `

  setTimeout(() => {
    if (new_div != null) {
      close_toast(new_div);
    }
  }, 6000);

  let parent = document.querySelector('.toast-container');
  parent.appendChild(new_div);
}

function toast(msg, toast_type) {
  let new_div = document.createElement("div");
  new_div.classList.add("toast");

  let img = "";

  if (toast_type == "success") {
    img = `  
    <svg version="1.2" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 50 50" class="toast-img">
    	<defs>
    		<image width="50" height="50" id="img1" href="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADIAAAAyCAMAAAAp4XiDAAAAAXNSR0IB2cksfwAAAAlwSFlzAAALEwAACxMBAJqcGAAAAFpQTFRFAHIAIIQgb69vn8qfv9y/////7/bvX6dfz+TPf7h/EHsQQJVAj8GPMI0wkMKQgLmA3+3fYKdgoMqgwNzAUJ5QsNOwP5U/r9Ov8PfwL4wvcLBwT55P4O7g0OXQRUVZOgAAAYZJREFUeJzt1dtSwjAQANANpYWGW7WDOo7//2kqIy0oFpGW1iDkspuk1RkfdMY8cGn3kG1Idhl8e7B/8gOEHYd4q6Fumq+QwcG8FjC27SCjw4FGVHUrGb87Em0iNBEivKZTnIMGGw/xCWJMMvQJEVbqpTPI9M0rxMrtHGS0bxEi62ebJEUrgUqmpsjFa7uASU5J+tJBINxiwktXFIsbvSazJSbOvILeFuKKZibJfO0WcLVSU+4xcTzKp4DrXF0oMYn0v9vvjXMlgp7+jeEGkVDdOG40EWgLiAoPGRwXIghtQclNpp4yfRCv3BaUGI9/MraQW8a1YmdDRfoIiNw+ATFUwDTDhPXNu2xeZlTA/B4TuFvQEDrk7lcEZeYaiQzQR6xrmkgeQU06zpg6YWa5aD1k6UJtQoPwy5bUIl0ZzDo2GWV27GmMV/ozKrBeYwpSxrncFGik1dL8SpvFLLYmSgrcYayWxEMcMN3RkuhofJzX0XmqpFrbnc/XXieidXkq7p/q+7+CfAB2rW8zzgjM+QAAAABJRU5ErkJggg=="/>
    	</defs>
    	<use id="check" href="#img1" transform="matrix(1,0,0,1,0,0)"/>
    </svg>
    `;
  } else if (toast_type == "warn") {
    img = `
    <svg version="1.2" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 50 50" class="toast-img">
    	<defs>
    		<image  width="50" height="50" id="img1" href="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADIAAAAyCAMAAAAp4XiDAAAAAXNSR0IB2cksfwAAAWhQTFRFjUoJklMVlVcbml8mm2Eoml4kk1QXjksLlVcat41l1Lqh6NrN69/U8Ofe8ejg7+Xc6dzP3827vZZwnmYvmFsgvZdx4dC/+vbz/////Pv56t3Rx6aFomw3kVAStoti38y6+/n36+DVvpdyl1ofxaOD/fz72sWwmVwi59nLsIJVllgcza+T8uriqHVDxqWE3su5tYpg+PTw1r6m0LSa0bac3sq4yquNklIUs4Za9vHsoGgyuI5m9/Luq3pLkVAR5dbHqndHnGIqr4BS1byj2cOt9O7o7ePYrX1Ot4xjq3lJs4Zb49PD5tfJp3RCn2cx1Lui9/PvkE4Pw5992sSv6t7T59nM/v38upFp7ODW28aynWQsl1oe8+3mnmUuyamK3ci1uZBpn2Ywv5l1xKGAvJVu/v79mFwh28Wx7eLXoms1soRYkVETpG876t7SsYNWoWkz8urj9e/q2cOupXA9nWMrsIFU9e/pupJqIOTYIAAAAgtJREFUeJxjZCAZMI5qwSYMAT+J1cLBiABvidAiAlKIYtsT/FpkGRl/oAqBrLyLRwuj8p8fGA7hZbz/G6cWdcYPGBpA4LvUPaSAQNaihc2zYCDyAclDSFrYlLDbAbZH7jIWLbKCr3DqYGAQvwn3JEKLwXNkR0oyMt7mQuLz8p/B0KLF/RjZVLlL+owPkAUUGY+ha7FmvIOsQvUwg90tFKfJXP6JpsX+JooCjQMMDBIoIiKi+1G1cEj8IKCFUXMfqhZRZhR5LFoYtPeianG5QlCL7m4ULYxcvAS1MLz+i6xFxOgSYS36O5G1sHBxoUpj06J8lGQtBjuQtTBzcRPWYrgdWQsbFweqtNE2BlZhNC1Kx5C1sLucRZU2Aebfk6hCFozrkbUwBB1jIASs1zKgaLF6/RlFnsOE8c5zFBE7xpWoWhgiDqAocGRkvIya5VwYF6Npidv9H1kBpvfd7x9C0yJjg2KNxwKGxO3IArwcsNyPyLxJh5F9oyvHuO0fEt/33VoGDC1pjBsZcIMAxumYWhhYuThx6tD9uw/ORi5Vsm5fxlQMAV6cU7FqYU9djN0eXdl5SDzUYlyX6RtqjIKAnTjj36lIfLTKgkOH9T6aDl557YkoAhhVktUvm60Im3x5GBl70VRgVnzsuYznjBdB7DRWOfz4MboCHNVr1NsvDH+ZxNdjkxtqTYWB1wIAhDl0M0QMVMIAAAAASUVORK5CYII="/>
    	</defs>
    	<use id="Background" href="#img1" x="0" y="0"/>
    </svg>
    `;
  } else if (toast_type == "remove") {
    img = `  
    <svg version="1.2" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 50 50" class="toast-img">
    	<defs>
    		<image  width="50" height="50" id="img1" href="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADIAAAAyCAMAAAAp4XiDAAAAAXNSR0IB2cksfwAAAVNQTFRFigUFjxERkxgYmCIimSQklyAgkBMTiwcHkhcXtmJi0qCg58zM6tPT797e8ODg7tzc6M7O3rm5u25unCsrlR0dvG9v4b6++fPz/////Pn56dDQxYODoDQ0jg4OtF9f3ri4+/f369TUvHBwlRwcxIGB/fv72a+vlh4e5srKrlJSkxkZzJGR8eLipkBAxYKC3be3s11d0Jqa3ba29uvrnS4utmNj9+3t3LS0zJKS0Z2d16qq+O/v5MbGmygov3Z2miYm7djYr1NTrVBQojk5zpeX06Ki2KysnzIy48TE+vX19Ofn1aWlq0xMtWBgqEZG48LC9+7ujQsLwXt716mp2a6usVdX/vz8uGdn69XV8+bmmyoqx4iIsFZW5cjIjg0NvXNz/v39lR4e2rCw7NfXsFVVjw8Pojg46dHRnjAw8uPj9enp2K2tozo6micnrlFR9OjouGhoE4LPIQAAAndJREFUeJxjZCAZMI5qwSYMAT+J1cLBiABvidAiAlKIYtsT/FpkGRl/oAqBrLyLRwuj8p8fGA7hZbz/G6cWdcYPGBpA4LvUPaSAQNaihc2zYCDyAclDSFrYlLDbAbZH7jIWLbKCr3DqYGAQvwn3JEKLwXN8KYGX5zyGFi2OZ3h0MDBIM55B12LK+AjKkmd89A/K/KaNiBTJaz/RtNjcgek4ycCh/xDMFBE8zGB3CyouInwQVQuHBMx7wlcZGGyZb4Dc8u06A4PjdZh7NPehahFlhlnH9A6o2fYmE4O0wF6gvPMVmITublQtbpfgHpVnO8zA4M745C/QfA5lOXhIGW1D0cLIxYsIHI2/QD0imiDC/CxCmOH1X2QtIjo3kOT+Ge6EeND533kkYZMtyFpYuLiQ5BgMzrwByWmhpjnZ03i0gEKXAR7WMGC6GVkLMxc3khw4dBlgYQ0HZpuQtbBxcSDpAIcuOzSsEUDmDLIWdo+TCFdBQpcHEtYX4OIWjOuRtTAEHYPJfNOHhi4krO2PwiRs1jCgaDH5+BnKUjwOC11wWIcehorbMa5E1cIQcQDKsF4LD11QWHPyQ8UdGZehaYne9x/CUGZ+Lc0KjUFrzrUh0JTFIK++CE2LjA3MGuyAlwOW+xGZN/7YZ+yKwcDjyxoYE6ElkXEbHi3eT3ZiamFg5eLEqcPoxz44G7lUSXl2DpcWd7bZWLWwx6/Ebo+R6EIkHmrZpcv0DTMM7ARvqcxG4qMVdxyaTE/RdPAqKs1AEcAoIU3+WuxB2OTBxcg4BU0FZqHKnsZ4XQuSnDhMZI68fIyuAEc5bMv/kkHhvux6bHJDrakw8FoAN+ycMzI25RUAAAAASUVORK5CYII="/>
    	</defs>
    	<use id="Background" href="#img1" x="0" y="0"/>
    </svg>    
    `;
  } else if (toast_type == "pause") {
    img = `  
    <svg version="1.2" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 50 50" class="toast-img">
    	<title>remove</title>
    	<defs>
    		<image width="50" height="50" id="img1" href="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADIAAAAyAgMAAABjUWAiAAAAAXNSR0IB2cksfwAAAAlwSFlzAAALEwAACxMBAJqcGAAAAAlQTFRFjUoJ////rX1OE0xagAAAAB5JREFUeJxjZEAGjAPOE3XgXgDCo7zBxKNFTFPOAwDo6joz4wKA6wAAAABJRU5ErkJggg=="/>
    	</defs>
    	<use id="pause" href="#img1" transform="matrix(1,0,0,1,0,0)"/>
    </svg>
    `;
  }
  
  new_div.innerHTML =
  `
    <div class="toast-title"> ` + img + `<b>` + msg + `</b></div>
    <b class="toast-close" onclick="close_toast(this.parentNode)">X</b> 
  `

  setTimeout(() => {
    if (new_div != null) {
      close_toast(new_div);
    }
  }, 6000);

  let parent = document.querySelector('.toast-container');
  parent.appendChild(new_div);
}

async function ping_backend() {
  
  await fetch(api_url + "/ping")
  .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    
    return response.text();
  })
  .then(data => {
    // console.log(data);

    if (unreachable) {
      console.log("regained contact!");
      
      close_popup_backend_unreachable();
      
      unreachable = false;
    }    
  })
  .catch(error => {
    // console.error('Fetch error:', error);

    unreachable_count += 1;

    if (unreachable_count > 2 && !unreachable) {      
      popup_backend_unreachable();

      unreachable = true;
    }
  });
}

function format_bytes(bytes) {
    if (bytes < 1024) {
        return bytes + ' B';
    } else if (bytes < 1024 * 1024) {
        return (bytes / 1024).toFixed(2) + ' KB';
    } else if (bytes < 1024 * 1024 * 1024) {
        return (bytes / (1024 * 1024)).toFixed(2) + ' MB';
    } else if (bytes < 1024 * 1024 * 1024 * 1024) {
        return (bytes / (1024 * 1024 * 1024)).toFixed(2) + ' GB';
    } else {
        return (bytes / (1024 * 1024 * 1024 * 1024)).toFixed(2) + ' TB';
    } 
}

function format_bytes_kb(bytes) {
  return (bytes / 1024).toFixed(2);
}

function format_bytes_mb(bytes) {
  return (bytes / (1024 * 1024)).toFixed(2);
}

function format_bytes_gb(bytes) {
  return (bytes / (1024 * 1024 * 1024)).toFixed(2);
}

function format_bytes_no_unit(bytes) {
    if (bytes < 1024) {
        return bytes;
    } else if (bytes < 1024 * 1024) {
        return (bytes / 1024).toFixed(2);
    } else if (bytes < 1024 * 1024 * 1024) {
        return (bytes / (1024 * 1024)).toFixed(2);
    } else {
        return (bytes / (1024 * 1024 * 1024)).toFixed(2);
    }
}

function deepEqual(obj1, obj2) {
  if (obj1 === obj2) {
    return true;
  }

  if (typeof obj1 !== 'object' || typeof obj2 !== 'object' || obj1 == null || obj2 == null) {
    return false;
  }

  const keys1 = Object.keys(obj1);
  const keys2 = Object.keys(obj2);

  if (keys1.length !== keys2.length) {
    return false;
  }

  for (let key of keys1) {
    if (!keys2.includes(key) || !deepEqual(obj1[key], obj2[key])) {
      return false;
    }
  }

  return true;
}

function arraysAreEqual(arr1, arr2) {
  if (arr1.length !== arr2.length) {
    return false;
  }

  for (let i = 0; i < arr1.length; i++) {
    if (!deepEqual(arr1[i], arr2[i])) {
      return false;
    }
  }

  return true;
}

async function open_link(url) {
  console.log("link pressed: " + url);
  
  await fetch(api_url + "/open_link", {method: 'POST', body: url})
  .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    
    return response.text();
  })
  .then(data => {
  })
  .catch(error => {
    popup("backend unreachable", "the backend may have crashed. restart the backend service and reload the page");
  });
}

function replace_links_with_divs(selector, base_path) {
  const parentElement = document.querySelector(selector);

  const links = parentElement.querySelectorAll('a');

  links.forEach(link => {
    const span = document.createElement('span');

    span.classList.add("link");

    let href = link.getAttribute("href");

    if (!href.startsWith("http://") && !href.startsWith("https://")) {
      if (base_path == null) {
        console.error("relative path is null");
      }

      href = base_path;
    }
    
    span.onclick = async function() {
      await open_link(href);
    }

    span.setAttribute("title", href);

    span.innerHTML = link.innerHTML;

    link.parentNode.replaceChild(span, link);
  });
}

async function main_load() {
  if (version_outdated) {
    toast_big("Version Outdated", "update to the latest version");
  }

  window.addEventListener('beforeunload', (event) => {
    console.log('The page is about to be unloaded');
  });
}
