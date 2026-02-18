// config.js
// 収入、月額固定費、年額固定費を定義します。

// 1. 収入のデフォルト項目
// ここに登録した項目が初期表示されます。
// HTML側で金額の編集や項目の追加・削除が可能です。
const INCOME_CONFIG = {
  salary: {
    label: "給与(目安)",
    amount: 165528
  },
  side_job: {
    label: "家賃補助",
    amount: 17000
  },
  new_1763422287706: {
    label: "ボーナス",
    amount: 0
  }
};

// 2. 月額固定費
// 毎月必ず支払う固定費
const FIXED_COSTS_CONFIG = {
  rent: {
    label: "家賃(25日目処)",
    amount: 50000
  },
  phone: {
    label: "携帯代(4886)",
    amount: 3278
  },
  insurance: {
    label: "アクサ生命(2237)",
    amount: 10000
  },
  subscription: {
    label: "FWD保険(2237)",
    amount: 5520
  },
  new_1768191291344: {
    label: "SBI(NISA)(4886)",
    amount: 5000
  },
  new_1768191312292: {
    label: "食費(現金)",
    amount: 30000
  },
  new_1768191318525: {
    label: "雑費(現金)",
    amount: 10000
  },
  new_1768191784043: {
    label: "BibleReality(2237)",
    amount: 2000
  },
  new_1769052848031: {
    label: "楽天ガス(来月まで）",
    amount: 2778
  },
  new_1769052896031: {
    label: "モバイルパスも",
    amount: 1000
  }
};

// 3. 年額固定費
// 年に1回など、毎月ではないが定期的に支払う費用
// HTML側で今月支払うかチェックを入れます
const ANNUAL_COSTS_CONFIG = {
  nhk: {
    label: "サーバー維持費",
    amount: 6336
  }
};

// ★削除: 4. 変動費のデフォルト項目

// --- 以下の行は変更しないでください ---
// グローバル変数として設定
window.INCOME_CONFIG = INCOME_CONFIG;
window.FIXED_COSTS_CONFIG = FIXED_COSTS_CONFIG;
window.ANNUAL_COSTS_CONFIG = ANNUAL_COSTS_CONFIG;
// ★削除: window.VARIABLE_COSTS_CONFIG = ...