import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';
import mermaid from 'astro-mermaid';
import { API_GROUPS } from './api/groups.mjs';
import { SECTIONS, SECTION_LABELS } from './sections.mjs';

export default defineConfig({
    site: "https://darkbriks.github.io/TLOR",
    base: "/TLOR",
    trailingSlash: 'always',
    build: {
        format: 'directory',
    },
    integrations: [
        mermaid({ theme: 'forest', autoTheme: true }),
        starlight({
            title: "TLOR",
            description: "Documentation for TLOR.",
            customCss: [
                '@liara/starlight-preset/styles/fonts.css',
                '@liara/starlight-preset/styles/tokens.css',
                '@liara/starlight-preset/styles/theme.css',
                '@liara/starlight-preset/styles/api.css',
                '@liara/starlight-preset/styles/hub.css',
            ],
            social: [{
                icon: 'github',
                label: 'GitHub',
                href: `https://github.com/darkbriks/TLOR`,
            }],
            editLink: {
                baseUrl: `https://github.com/darkbriks/TLOR/edit/main/`,
            },
            routeMiddleware: './src/routeData.ts',
            disable404Route: true,
            components: {
                SiteTitle: '@liara/starlight-preset/components/SiteTitle.astro',
                ThemeSelect: '@liara/starlight-preset/components/ReadingPreferences.astro',
                Head: '@liara/starlight-preset/components/Head.astro',
                PageTitle: '@liara/starlight-preset/components/PageTitle.astro',
            },
            sidebar: [
                {
                    label: SECTION_LABELS[SECTIONS.about],
                    items: [{ autogenerate: { directory: SECTIONS.about } }],
                },
                {
                    label: SECTION_LABELS[SECTIONS.guide],
                    items: [{ autogenerate: { directory: SECTIONS.guide } }],
                },
                {
                    label: SECTION_LABELS[SECTIONS.api],
                    items: [
                        { label: 'Overview', link: `${SECTIONS.api}/` },
                        ...API_GROUPS.map((group) => ({
                            label: group.label,
                            items: [{ autogenerate: { directory: `${SECTIONS.api}/${group.dir}` } }],
                        })),
                    ],
                },
            ],
            pagination: true,
            lastUpdated: true,
            plugins: []
        }),
    ],
})
