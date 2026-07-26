const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");


const inspectorPath = path.resolve(__dirname, "../ui/inspector.js");
const context = vm.createContext({
    console,
    network: {classinfo: {}},
});
vm.runInContext(
    fs.readFileSync(inspectorPath, "utf8") +
        "\nglobalThis.inspectorUnderTest = inspector;\n",
    context,
    {filename: inspectorPath},
);
const inspector = context.inspectorUnderTest;

assert.equal(inspector.safeMarkdownUrl("javascript:alert(1)", false), "");
assert.equal(inspector.safeMarkdownUrl("data:text/html,x", true), "");
assert.equal(inspector.safeMarkdownUrl("https://example.test/a?q=1", false),
             "https://example.test/a?q=1");
assert.equal(inspector.safeMarkdownUrl("docs/file.md", false), "docs/file.md");

const rendered = inspector.renderInlineMarkdown(
    '[unsafe](javascript:alert(1)) ![bad](data:text/html,x) **åäö**',
);
assert.equal(rendered.includes("javascript:"), false);
assert.equal(rendered.includes("data:text/html"), false);
assert.equal(rendered.includes("<strong>åäö</strong>"), true);

console.log("escaping tests passed");
