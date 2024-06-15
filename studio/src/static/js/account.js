let eth_balance_main = "~";
let eth_balance_base = "~";

let shog_balance_main = "~";
let shog_balance_base = "~";

let shog_rewards_main = "~";
let shog_rewards_base = "~";

let votes;

let eth_input;

let buy_rate = 360000;

let template_str = `
<div class="account-address">
  <div class="account-address-title">wallet address:</div>
  <div class="account-address-value">{{this.wallet_address}}</div>
</div>

<div class="balance-view">
  <div class="balance-row">
    <div class="balance-row-item">
      <div class="balance-row-item-title">$SHOG Balance (MAINNET)</div>
      <div class="balance-row-item-value shog-balance-main"><img class="balance-loading" src="/static/img/loading.gif"/></div>
      <div class="buy-btn" onclick="buy_shog('MAINNET')">Buy $SHOG</div>
    </div>

    <div class="balance-row-item">
      <div class="balance-row-item-title">$SHOG Balance (BASE)</div>
      <div class="balance-row-item-value shog-balance-base"><img class="balance-loading" src="/static/img/loading.gif"/></div>
      <div class="buy-btn" onclick="buy_shog('BASE')">Buy $SHOG</div>
    </div>
  </div>

  <div class="balance-row">
    <div class="balance-row-item">
      <div class="balance-row-item-title">$ETH Balance (MAINNET)</div>
      <div class="balance-row-item-value eth-balance-main"><img class="balance-loading" src="/static/img/loading.gif"/></div>
    </div>

    <div class="balance-row-item">
      <div class="balance-row-item-title">$ETH Balance (BASE)</div>
      <div class="balance-row-item-value eth-balance-base"><img class="balance-loading" src="/static/img/loading.gif"/></div>
    </div>
  </div>

  <div class="balance-row">
    <div class="balance-row-item">
      <div class="balance-row-item-title">$SHOG Rewards Earned (MAINNET)</div>
      <div class="balance-row-item-value rewards-main"><img class="balance-loading" src="/static/img/loading.gif"/></div>
    </div>

    <div class="balance-row-item">
      <div class="balance-row-item-title">$SHOG Rewards Earned (BASE)</div>
      <div class="balance-row-item-value rewards-base"><img class="balance-loading" src="/static/img/loading.gif"/></div>
    </div>
  </div>

  <div class="votes-title">Leaderboard Votes</div>

  <div class="votes">
    {{#each this.votes}}
      <div class="votes-subtitle">{{this.[0]}}</div>

      <div class="vote-item">
        <a href="/leaderboards" class="item-name"><img class="item-image" src="{{../hub_url}}/static/img/users/{{this.[1].namespace}}.png"/>{{this.[1].namespace}}/{{this.[1].name}}</a>
      </div>
    {{else}}
      no votes
    {{/each}}
  </div>
</div>
`;

async function buy_final_pressed(chain) {
  console.log("buy final pressed");

  document.querySelector(".buy-btn-final").innerHTML = "<img src='/static/img/spinner.gif' class='buy-loading'/>";

  let final_chain = chain.toLowerCase();
  
  await fetch(api_url + `/buy_shog/${final_chain}/` + eth_input)
  .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    
    return response.text();
  })
  .then(data => {
    toast("Transaction Successful", "success");

    popup_close_clicked(document.querySelector(".buy-popup"));

    setInterval(reload, 2000);
  })
  .catch(error => {
    toast("Transaction Failed", "warn");

    document.querySelector(".buy-btn-final").innerHTML = "Buy";
  });  
}

function handle_eth_key_press(event) {
  console.log("eth value changed");

  let element = document.querySelector(".input-eth");

  const value = element.value;

  if (isNaN(value)) {
    element.value = eth_input;
  } else {
    eth_input = value;
  }

  update_shog_value();
}

function update_shog_value() {
  let new_value = (eth_input * buy_rate);

  document.querySelector(".buy-shog-value").innerHTML = "= " + new_value.toLocaleString() + " $SHOG";
}

function buy_shog(chain) {  
  let title = `Buy $SHOG (${chain})`;

  let msg;

  let balance = 0;

  if (chain == "MAINNET") {
    balance = eth_balance_main;
  } else {
    balance = eth_balance_base;
  }

  eth_input = balance;

  let shog_value = (balance * buy_rate).toLocaleString();
    
  msg = `
    <div class="buy-balance">ETH Balance (${chain}): ${balance}</div>
  `;

  msg = msg + `
    <div class="buy-eth-value">
      <input class="input-eth" value="${eth_input}" onkeyup="handle_eth_key_press(event)"/>    
      <div class="eth-unit">ETH</div>
    </div>

    <div class="buy-shog-value"> = 0 $SHOG</div>
  
    <div class="buy-btn buy-btn-final" onclick="buy_final_pressed('${chain}')">Buy</div>  
  `;
  
  let new_div = document.createElement("div");
  new_div.classList.add("popup");
  new_div.classList.add("buy-popup");
  
  new_div.innerHTML =
  ` 
  <div class="popup-outer" onclick="popup_outer_clicked(this.parentNode)"></div>
  <div class="popup-inner popup-inner-buy">
    <div class="popup-title"><b>` + title + `</b></div>
    <div class="popup-msg popup-msg-buy">` + msg + `</div>
  </div>
  `

  let parent = document.getElementById('content');
  parent.appendChild(new_div);

  update_shog_value();
}

function set_info() {
  console.log(wallet_address);

  let template_data = {
    hub_url, 
    wallet_address, 
    votes, 
  };
  
  const template = Handlebars.compile(template_str);
  let compiled = template(template_data);

  const parent = document.querySelector(".main-content");
  parent.innerHTML = compiled;
 }

async function set_balance() {
  
    // shog_balance_main: shog_balance_main.toLocaleString(), 
    // eth_balance_main: eth_balance_main.toLocaleString(), 
    // shog_balance_base: shog_balance_base.toLocaleString(), 
    // eth_balance_base: eth_balance_base.toLocaleString(), 
    // shog_rewards_main: shog_rewards_main.toLocaleString(), 
    // shog_rewards_base: shog_rewards_base.toLocaleString()

  document.querySelector(".eth-balance-main").innerHTML = eth_balance_main.toLocaleString();
  document.querySelector(".eth-balance-base").innerHTML = eth_balance_base.toLocaleString();
  
  document.querySelector(".shog-balance-main").innerHTML = shog_balance_main.toLocaleString();
  document.querySelector(".shog-balance-base").innerHTML = shog_balance_base.toLocaleString();

  document.querySelector(".rewards-main").innerHTML = shog_rewards_main.toLocaleString();
  document.querySelector(".rewards-base").innerHTML = shog_rewards_base.toLocaleString();
}

async function load_balance() {
  await fetch(api_url + "/get_crypto_balance/" + wallet_address)
  .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    
    return response.text();
  })
  .then(data => {
    console.log(data);

    eth_balance_main = parseFloat(JSON.parse(data).eth_main);
    shog_balance_main = parseFloat(JSON.parse(data).shog_main);
    
    eth_balance_base = parseFloat(JSON.parse(data).eth_base);
    shog_balance_base = parseFloat(JSON.parse(data).shog_base);

    shog_rewards_main = parseFloat(JSON.parse(data).rewards_eth);
    shog_rewards_base = parseFloat(JSON.parse(data).rewards_base);
  })
  .catch(error => {
    console.error('Fetch error:', error);
  });  

  await set_balance();  
}

async function load() {    
  await fetch(hub_url + "/api/get_node_leaderboard_votes/" + node_id)
  .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    
    return response.text();
  })
  .then(data => {
    // console.log(data);

    votes = JSON.parse(data);
  })
  .catch(error => {
    console.error('Fetch error:', error);
  });  
  
  set_info();

  await load_balance();
}


window.onload = async function() {
  await main_load();
  
  await load();

  setInterval(ping_backend, ping_rate);
}

