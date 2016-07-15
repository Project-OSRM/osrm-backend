module.exports = function(source, access_tags_hierarchy) {
    for (let v of access_tags_hierarchy) {
        const tag = source.tag(v);
        if (tag) {
            return tag;
        }
    }
    return '';
};
