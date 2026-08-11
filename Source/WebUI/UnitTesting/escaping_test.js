const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");


const inspectorPath = path.resolve(__dirname, "../ui/inspector.js");
const warnings = [];
const context = vm.createContext({
    console: {...console, warn: (...args) => warnings.push(args)},
    network: {classinfo: {}},
});
vm.runInContext(
    fs.readFileSync(path.resolve(__dirname, "../js/katex/katex.min.js"), "utf8"),
    context,
    {filename: "katex.min.js"},
);
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

const inlineMath = inspector.renderInlineMarkdown("Voltage \\(V_i\\) and `literal \\(x\\)`. ");
assert.equal(inlineMath.includes('class="katex"'), true);
assert.equal(inlineMath.includes("literal \\(x\\)"), true);

const displayMath = inspector.renderMarkdown("\\[x^2 + y^2 = z^2\\]");
assert.equal(displayMath.includes('class="katex-display"'), true);

const multilineMath = inspector.renderMarkdown("\\[\nx^2\n- y^2\n\\]");
assert.equal(multilineMath.includes('class="katex-display"'), true);
assert.equal(multilineMath.includes("<li>"), false);

const codeMath = inspector.renderMarkdown("```text\n\\[not_math\\]\n```");
assert.equal(codeMath.includes('class="katex"'), false);
assert.equal(codeMath.includes("\\[not_math\\]"), true);

const invalidMath = inspector.renderInlineMarkdown("\\(\\notacommand{\\)");
assert.equal(invalidMath.includes('class="katex"'), false);
assert.equal(invalidMath.includes("\\(\\notacommand{\\)"), true);
assert.equal(warnings.length, 1);

const untrustedMath = inspector.renderInlineMarkdown("\\(\\href{javascript:alert(1)}{x}\\)");
assert.equal(untrustedMath.includes("<a "), false);
assert.equal(untrustedMath.includes('href="javascript:'), false);

console.log("escaping tests passed");
