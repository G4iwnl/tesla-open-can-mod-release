import {themes as prismThemes} from 'prism-react-renderer';
import type {Config} from '@docusaurus/types';
import type * as Preset from '@docusaurus/preset-classic';

const config: Config = {
  title: 'Tesla Open CAN Mod',
  tagline: 'Open-source CAN bus modification tool for Tesla vehicles',
  favicon: 'img/favicon.ico',

  future: {
    v4: true,
  },

  url: 'https://teslaopencanmod.org',
  baseUrl: '/',

  onBrokenLinks: 'throw',

  i18n: {
    defaultLocale: 'en',
    locales: ['en'],
  },

  presets: [
    [
      'classic',
      {
        docs: {
          sidebarPath: './sidebars.ts',
          editUrl:
            'https://gitlab.com/tesla-open-can-mod/tesla-open-can-mod/-/edit/main/docs-site/',
        },
        blog: false,
        theme: {
          customCss: './src/css/custom.css',
        },
      } satisfies Preset.Options,
    ],
  ],

  themeConfig: {
    image: 'img/social-card.jpg',
    colorMode: {
      defaultMode: 'dark',
      respectPrefersColorScheme: true,
    },
    navbar: {
      title: 'Tesla Open CAN Mod',
      items: [
        {
          type: 'docSidebar',
          sidebarId: 'docsSidebar',
          position: 'left',
          label: 'Docs',
        },
{
          href: 'https://discord.gg/ZTQKAUTd2F',
          label: 'Discord',
          position: 'right',
        },
        {
          href: 'https://gitlab.com/tesla-open-can-mod/tesla-open-can-mod',
          label: 'GitLab',
          position: 'right',
        },
      ],
    },
    footer: {
      style: 'dark',
      links: [
        {
          title: 'Documentation',
          items: [
            {
              label: 'Getting Started',
              to: '/docs/getting-started/hardware-selection',
            },
            {
              label: 'Features',
              to: '/docs/features/fsd-activation',
            },
            {
              label: 'Hardware',
              to: '/docs/hardware/esp32',
            },
          ],
        },
        {
          title: 'Community',
          items: [
            {
              label: 'Discord',
              href: 'https://discord.gg/ZTQKAUTd2F',
            },
            {
              label: 'GitHub',
              href: 'https://github.com/ylovex75/tesla-open-can-mod-release',
            },
          ],
        },
        {
          title: 'Legal',
          items: [
            {
              label: 'Disclaimer',
              to: '/docs/legal/disclaimer',
            },
            {
              label: 'License (GPL-3.0)',
              href: 'https://www.gnu.org/licenses/gpl-3.0.html',
            },
          ],
        },
      ],
      copyright: `Copyright © ${new Date().getFullYear()} Tesla Open CAN Mod Contributors. Licensed under GPL-3.0.`,
    },
    prism: {
      theme: prismThemes.github,
      darkTheme: prismThemes.dracula,
      additionalLanguages: ['bash', 'cpp'],
    },
  } satisfies Preset.ThemeConfig,
};

export default config;
