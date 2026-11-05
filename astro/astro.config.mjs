import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';
import mermaid from 'astro-mermaid';
import { API_GROUPS } from './api/groups.mjs';

export const SECTIONS = Object.freeze({ about: 'about', guide: 'guides', api: 'api' });

export default defineConfig({
    site: "https://darkbriks.github.io/TLOR/dev",
    base: "/TLOR/dev",
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
                baseUrl: `https://github.com/darkbriks/TLOR/edit/main/docs/`,
            },
            components: {
                SiteTitle: '@liara/starlight-preset/components/SiteTitle.astro',
                ThemeSelect: '@liara/starlight-preset/components/ReadingPreferences.astro',
                Head: '@liara/starlight-preset/components/Head.astro',
                PageTitle: '@liara/starlight-preset/components/PageTitle.astro',
                Sidebar: '@liara/starlight-preset/components/Sidebar.astro',
                MobileMenuFooter: '@liara/starlight-preset/components/MobileMenuFooter.astro',
            },
            sidebar: [
                {
                    label: 'About',
                    items: [{ autogenerate: { directory: SECTIONS.about } }],
                },
                {
                    label: 'Guide',
                    items: [{ autogenerate: { directory: SECTIONS.guide } }],
                },
                {
                    label: 'API reference',
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
