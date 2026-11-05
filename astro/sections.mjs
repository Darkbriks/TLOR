export const SECTIONS = Object.freeze({ about: 'about', guide: 'guides', api: 'api' });

export const SECTION_LABELS = Object.freeze({
    [SECTIONS.about]: 'About',
    [SECTIONS.guide]: 'Guide',
    [SECTIONS.api]: 'API reference',
});

export const SECTION_SOURCES = Object.freeze({
    [SECTIONS.about]: '',
    [SECTIONS.guide]: 'docs/',
});
