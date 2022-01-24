
class DataLanguagePlugin {
    constructor(options) {
        self.prefix = options.dataPrefix;
        self.found = [];
    }
    'before:highlightElement' ({el, language}) {
        el.innerHTML = el.innerHTML.replaceAll(
            /^([\s]*)&lt;&lt;([^:\s]+?)&gt;&gt;([\s]*)$/mg,
            function (match, p1, p2, p3, offset, string){
                var index = self.found.length;
                self.found.push(`${p1}<a class="lp-ref" href="#lp-${p2}">${p2}</a>${p3}`);
                return `«x${index}x»`;
            }
        ).replaceAll(
            /^([\s]*)&lt;&lt;([^:\s]+::[^:\s]+?)&gt;&gt;([\s]*)$/mg,
            function (match, p1, p2, p3, offset, string){
                const index = self.found.length;
                self.found.push(`${p1}<span class="lp-ref">${p2}</span>${p3}`);
                return `«x${index}x»`;
            }
        );
    }
    'after:highlightElement' ({ el, result, text }) {
        for (const index in self.found) {
            el.innerHTML = el.innerHTML.replace(`«x${index}x»`, self.found[index]);
        }
    }
}
  
hljs.addPlugin(new DataLanguagePlugin({ dataPrefix: 'hljs' }));

hljs.highlightAll();

