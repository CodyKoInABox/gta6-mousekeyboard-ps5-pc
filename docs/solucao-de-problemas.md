# PS5 Keyboard Bridge: solução de problemas

Procure o sintoma abaixo. As mensagens entre aspas são as que o app mostra.

## O PS5 não aparece em "Procurar PS5"

- O PS5 está ligado ou em repouso? Em repouso, precisa de **Manter conectado à Internet** e **Ativar ligação do PS5 pela rede** ligados.
- PC e PS5 estão na mesma rede? Visitantes de Wi-Fi ou rede de convidados costumam isolar os aparelhos.
- O Firewall do Windows pode estar bloqueando a busca. Na primeira vez que o app usa a rede, o Windows pergunta se pode liberar. Escolha **Redes privadas** e **Permitir**. Se você negou sem querer, procure "Permitir um aplicativo pelo Firewall do Windows" no menu Iniciar e marque o app.
- Digite o IP do PS5 no campo "Não apareceu?". O IP fica em **Configurações > Rede > Ver status da conexão** no PS5.

## "O PS5 recusou a conexão. Ele leva cerca de 10 segundos para aceitar uma nova sessão."

Espere 10 segundos e clique em **JOGAR** de novo. O auto-reconectar também tenta sozinho.

## "Outro Remote Play (PS Remote Play no PC ou no celular, ou Chiaki) já está conectado ao PS5. Feche-o e tente de novo."

O PS5 só aceita uma sessão de Remote Play por vez. Feche:

- o PS Remote Play no PC;
- o app PS App ou o PS Remote Play no celular (Remote Play pelo celular);
- o Chiaki, se você usa.

Depois clique em **JOGAR** de novo. Se continuar, reinicie a sessão de Remote Play no PS5 ou reinicie o PS5.

## "Outro app de Remote Play está aberto" (aviso antes de jogar)

Ao clicar em **JOGAR**, o app procura processos de Remote Play abertos no PC (PS Remote Play e Chiaki) antes de conectar. Se achar algum, mostra o nome dele e pergunta se você quer continuar.

- Feche o outro app e clique em **JOGAR** de novo. É o caminho certo na maioria dos casos.
- Se você sabe que o outro app não está conectado ao PS5, escolha **Sim** para tentar mesmo assim.

O app não consegue ver sessões de Remote Play abertos no celular. Se a conexão for recusada, feche o Remote Play do celular também.

## Status "Sem rede" ("Sem conexão de rede. Verifique o Wi-Fi ou o cabo; o app tenta de novo sozinho.")

O PC não tem uma conexão de rede utilizável. O app não tenta conectar sem rede: ele confere de novo a cada 3 segundos e retoma sozinho quando o Wi-Fi ou o cabo voltar.

Se o status não sair de "Sem rede" com a rede ligada:

- confira se o Windows tem um endereço IPv4 válido. No PowerShell, rode `ipconfig` e procure **Endereço IPv4** no adaptador que você usa. Um endereço `169.254.x.x` indica que o Windows não recebeu IP do roteador, então conta como sem rede;
- desative e ative o adaptador de rede, ou reconecte o Wi-Fi.

## "Não foi possível conectar ao PS5 pela rede. Confira se o PS5 e o PC estão na mesma rede."

- Teste se o PC enxerga o PS5: no PowerShell, rode `ping <IP do PS5>`.
- Desligue VPN no PC durante o jogo. VPN costuma bloquear o tráfego local do Remote Play.
- Se o Wi-Fi do PS5 estiver ruim, use cabo.

## "O pareamento salvo não é mais válido. Pareie o PS5 de novo nas configurações."

O PS5 esqueceu o PC, ou o Remote Play foi desativado e reativado. Abra **Configuração inicial** e refaça o pareamento com um novo código de 8 dígitos.

## O código de 8 dígitos não é aceito

- Os códigos expiram após alguns minutos. Na tela do PS5, **Link Device**, peça um código novo.
- Digite só os números. O app aceita espaços ou traços (`1234 5678`).
- Se o pareamento antigo ainda estiver salvo e não funcionar, apague-o com **Esquecer PS5** em Configurações e pareie de novo.

## Não consigo fazer login na Sony

- Use o botão **Abrir login da Sony**, que abre o site oficial no navegador padrão.
- Depois do login, copie a URL inteira da página final ("redirect"), não só o endereço principal, e cole no campo.
- Se o navegador não abrir, o app mostra o endereço para você copiar.
- O app nunca pede nem guarda a sua senha.

## "O PS5 encerrou a sessão."

O PS5 foi desligado, entrou em repouso, ou abriu outro jogo ou aplicativo de Remote Play. Com auto-reconectar ligado, o app tenta de novo. Se o PS5 estiver em repouso, acorde-o antes.

## "Erro de Remote Play. Tente novamente."

Tente de novo. Se acontecer toda vez, anote o horário e a mensagem exata, e envie junto o modelo do PS5 e a versão do firmware, para abrir uma issue.

## Não consigo ver a imagem no Veyra

O app não mexe no vídeo. A imagem vem da capturadora, como antes. Se a tela do Veyra não aparece:

- confira a entrada HDMI e o Veyra NRVideo do seu lado, sem relação com este app;
- o app só troca o foco para o Veyra ao jogar, ele não altera a configuração dele.

## O app não acha o Veyra

- Abra o Veyra uma vez, depois use **Configurações > Detectar** no app.
- Se continuar, use **Procurar...** em Configurações e escolha o `Veyra.exe`.

## As teclas do jogo também acionam coisas no Veyra

A captura só bloqueia teclas e cliques enquanto está **ativa** (o aviso de captura na tela principal mostra "ativa"). Se algo escapar:

- confira se a captura está ativa, e não só "aguardando o Veyra em foco";
- pressione F8 para desligar a captura e voltar ao normal.

## Não consigo sair de um jogo travado no PS5 (botão preso)

- Pressione **Esc**. Ele solta todos os botões e desliga a captura.
- Se nada acontecer, clique em **PARAR**.
- Se o app travou, feche-o pelo gerenciador de tarefas e reinicie o PS5 se o personagem continuar andando.

## O cursor do mouse não se move no Windows

Se a captura ficou presa, pressione **Esc** ou **F8**. Quando o app fecha, o Windows libera o cursor.

## A mira no jogo está muito lenta, rápida ou tremida

- Abra **Configurações > Ajustes avançados de mira** e ajuste a sensibilidade. A câmera (sem mirar) e a mira com L2 têm valores separados.
- Se a mira ficar tremida, aumente a suavização (ms) aos poucos. Se ela atrasar demais, diminua.
- Zona morta alta demais faz o analógico ficar parado com movimentos pequenos.
- Alguns jogos limitam a velocidade de giro do analógico direito. Isso acontece no jogo, não no app.

## O Windows mostra "O Windows protegeu o computador"

O instalador ainda não tem assinatura digital. Clique em **Mais informações** e depois em **Executar assim mesmo**. Confira o hash SHA-256 antes, como mostrado no guia de configuração.

## Windows 10

Para ver a versão exata, aperte Win+R, digite `winver` e Enter. O número da build aparece na janela.

- **Windows 11, ou Windows 10 build 17763 (versão 1809) ou mais nova**: compatível. O app foi testado no Windows 10 build 19045 (versão 22H2).
- **Windows 10 anterior à build 17763**: o app abre e mostra o aviso "Windows 10 build N é anterior ao 1809 ... e não foi testado". Pode funcionar. Se algo falhar, atualize o Windows em **Configurações > Atualização e Segurança**, e abra uma issue com a versão exata.
- **Windows 8.1 ou anterior, ou Windows de 32 bits**: o app não abre e mostra "Este app precisa de Windows 10 ou 11 de 64 bits."

O assistente mostra o mesmo status na tela de boas-vindas.

## Onde ficam os dados

- Dados, configurações e pareamento: `%LOCALAPPDATA%\PS5KeyboardBridge`.
- O app ainda não grava um arquivo de log. Ao reportar um problema, copie a mensagem da tela e o horário.
- Para apagar tudo: desinstale o app e remova essa pasta.
