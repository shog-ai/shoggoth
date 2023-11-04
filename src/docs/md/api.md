{{> head}}

<h1 class="page-title"> Shoggoth Node API Reference </h1>

{{#for apis}}

<div class="api-item">
<div class="request">
<h4>Endpoint</h4>
<div class="endpoint">
{{this.endpoint}}
</div>

<h4>Description</h4>
<div class="request-description">
{{this.description}}
</div>

<h4>Required Headers</h4>
<div class="headers">

{{#for this.headers}}

<div class="header">
<div class="header-key">{{this.key}}</div>
<div class="header-value">{{this.value}}</div>
</div>

{{/for}}

</div>

<h4>Body</h4>
<div class="request-body">

{{this.request_body}}

</div>
</div>

<hr>

<h3>Responses</h3>

<div class="response"> 


{{#for this.responses}}

<div class="response-item">
<h4>{{this.status}}</h4>

<h4>Description</h4>
<div class="request-description">
{{this.description}}
</div>

<h4>Headers</h4>
<div class="headers">

{{#for this.headers}}
<div class="header">
<div class="header-key">{{this.key}}</div>
<div class="header-value">{{this.value}}</div>
</div>
{{/for}}

</div>

<h4>Body</h4>
<div class="request-body">
{{this.body}}
</div>
</div>

{{/for}}



</div>
</div>



{{/for}}

{{> end}}

