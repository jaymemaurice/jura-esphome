// Parse every inline <script> from the real embedded HTML without dependencies.
// A new Function detects syntax, does not execute browser code.
const fs=require('fs');
const html=fs.readFileSync('web/jura-web-console.html','utf8');
const scripts=[...html.matchAll(/<script(?:\s[^>]*)?>([\s\S]*?)<\/script>/gi)];
if(!scripts.length) throw Error('No embedded browser JavaScript found');
for(const [i,m] of scripts.entries()) {
  // Parser only; this function is never invoked.
  new Function(m[1]);
  console.log(`Inline browser JS ${i+1}: valid syntax (${m[1].length} chars)`);
}
