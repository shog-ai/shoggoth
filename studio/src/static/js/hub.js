let resources;

let template_str = `
  {{#each items}}

 
          <div class="main-item main-item-{{this.category}}">
            <a href="/hub/resource/{{this.namespace}}/{{this.name}}" class="main-item-top">
              <div class="main-item-user">
                <img class="user-img" src="{{../hub_url}}/static/img/users/{{this.namespace}}.png"/>
                <div class="main-item-title">{{this.namespace}} / {{this.name}}</div>
              </div>
        
              <div class="main-item-top-last">
                <div class="main-item-type">
                  <img class="main-item-model-icon" src="/static/img/{{this.category}}.svg"/>
                  <div class="main-item-type-txt">{{this.category}}</div>
                </div>          
              </div>
            </a>
        
          </div>
    
        {{else}}
          <div>no resources</div>  
        {{/each}}
`;

let last_search_update_time;
let edited = false;

function search_key_pressed(event) {
  // console.log("search key pressed");

  last_search_update_time = Date.now();

  edited = true;
}

async function search_string(search_query) {
  if (search_query == "") {
    load();
    return;
  }
  
  const parent = document.querySelector(".hub-items");
  parent.innerHTML = '<img src="/static/img/loading.gif" class="loading" id="loading" />';
  document.querySelector(".home-banner").innerHTML = "";

  let url = settings.shog_hub_url + "/api/search_string";
  
  await fetch(url, {method: 'POST', body: search_query})
  .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    
    return response.text();
  })
  .then(data => {
    // console.log(data);

    resources = JSON.parse(data);

    const banner = document.querySelector(".home-banner");
    banner.innerHTML = `search results for "` + search_query + '"';

    set_resources();
  })
  .catch(error => {
    console.error('Fetch error:', error);
  });  
}

async function search_category(category) {
  const parent = document.querySelector(".hub-items");
  parent.innerHTML = '<img src="/static/img/loading.gif" class="loading" id="loading" />';
  document.querySelector(".home-banner").innerHTML = "";

  let url = settings.shog_hub_url + "/api/get_" + category;
  
  await fetch(url)
  .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    
    return response.text();
  })
  .then(data => {
    // console.log(data);

    resources = JSON.parse(data);

    const banner = document.querySelector(".home-banner");
    banner.innerHTML = category + ` on <img class="home-banner-img" src="/static/img/icon.png"/>`;

    set_resources();
  })
  .catch(error => {
    console.error('Fetch error:', error);
  });  
}

async function search_tag(tag) {
  const parent = document.querySelector(".hub-items");
  parent.innerHTML = '<img src="/static/img/loading.gif" class="loading" id="loading" />';
  document.querySelector(".home-banner").innerHTML = "";

  let url = settings.shog_hub_url + "/api/search_tag/" + tag;
  
  await fetch(url)
  .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    
    return response.text();
  })
  .then(data => {
    // console.log(data);

    resources = JSON.parse(data);

    const banner = document.querySelector(".home-banner");
    banner.innerHTML = `search results for "` + tag + '" tag';

    set_resources();
  })
  .catch(error => {
    console.error('Fetch error:', error);
  });  
}

async function check_search() {
  if (edited) {
    let now = Date.now();
    let elapsed = now - last_search_update_time;

    if (last_search_update_time != 0 && elapsed > 1500) {
      console.log("searching!!!");

       let search_query = document.querySelector(".filter-search").value;
       await search_string(search_query);

      edited = false;
    } else {
      const parent = document.querySelector(".hub-items");
      parent.innerHTML = '<img src="/static/img/loading.gif" class="loading" id="loading" />';
      document.querySelector(".home-banner").innerHTML = "";
    }
  }
}

function set_resources() {
  // console.log(items);

  resources.items.forEach((item) => {
    item.category = item.category.toLowerCase();
  });
  
  resources.hub_url = hub_url;
    
  const template = Handlebars.compile(template_str);
  let compiled = template(resources);
    
  // console.log(compiled);
  
  const parent = document.querySelector(".hub-items");
  parent.innerHTML = compiled;
}

  
async function load() {
  let route = window.location.pathname.split('/').slice(-1)[0];

  const banner = document.querySelector(".home-banner");
  if (route == "hub") {
    banner.innerHTML = `
      Trending on
      <img class="home-banner-img" src="/static/img/icon.png"/>
      this week
    `;
  } else if (route == "model") {
    banner.innerHTML = `Models on <img class="home-banner-img" src="/static/img/icon.png"/>`;
  } else if (route == "code") {
    banner.innerHTML = `Code on <img class="home-banner-img" src="/static/img/icon.png"/>`;
  } else if (route == "dataset") {
    banner.innerHTML = `Datasets on <img class="home-banner-img" src="/static/img/icon.png"/>`;
  } else if (route == "paper") {
    banner.innerHTML = `Papers on <img class="home-banner-img" src="/static/img/icon.png"/>`;
  }
  
  let url;

  if (route == "hub") {
    url = settings.shog_hub_url + "/api/get_trending";
  } else if (route == "model") {
    url = settings.shog_hub_url + "/api/get_models";
  } else if (route == "code") {
    url = settings.shog_hub_url + "/api/get_code";
  } else if (route == "dataset") {
    url = settings.shog_hub_url + "/api/get_datasets";
  } else if (route == "paper") {
    url = settings.shog_hub_url + "/api/get_papers";
  }
  
  await fetch(url)
  .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    
    return response.text();
  })
  .then(data => {
    // console.log(data);

    resources = JSON.parse(data);

    set_resources();
  })
  .catch(error => {
    console.error('Fetch error:', error);
  });
}

window.onload = async function() {
  await main_load();
  
  let window_url = window.location.href;
  let url = new URL(window_url);
  let params = url.searchParams;
  
  document.querySelector(".filter-search").value = "";

  if (params.size > 0) {
    console.log("has params!!!");

    if (params.has('model')) {
      search_category("models");
    } else if (params.has('code')) {
      search_category("code");
    } else if (params.has('dataset')) {
      search_category("datasets");
    } else if (params.has('paper')) {
      search_category("papers");
    } else if (params.has('tag')) {
      search_tag(params.get("tag"));
    } else if (params.has('search')) {
      let search_query =  params.get("search");
      document.querySelector(".filter-search").value = search_query;
      search_string(search_query);
    }
  } else {
    load();
  }
  
  setInterval(check_search, 500);

  setInterval(ping_backend, ping_rate);
}

