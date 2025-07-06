#include "button.h"

/**
 * @param button ����İ������ƿ�
 * @param idleLevel ����û�а���ʱ�Ŀ��е�ƽ
 * @param doublePress_Threshold ��һ�ΰ���������ɺ�ֹͣ�Եڶ��ΰ������µĵȴ�ת��ֱ���ж�Ϊ���������ʱ����ֵ����λΪ10ms���Ƽ�20
 * @param longPress_Threshold   ��һ�ΰ��²��ɿ�һֱ���ж�Ϊ��������ֵ����λ10ms���Ƽ�100,
 *                             
 * @param getButtonVal �����Լ�ʵ�ֵİ���״̬���ص�����
 */
void buttonInit(button_t *button, idleLevel_t idleLevel, uint16_t doublePress_Threshold, uint16_t longPress_Threshold, getButtonVal_t getButtonVal)
{
    button->val = idleLevel;
    button->valPre = idleLevel;
    button->ticks = 0;    
    button->idleLevel = idleLevel;
    button->doublePress_Threshold = doublePress_Threshold;
    button->longPress_Threshold = longPress_Threshold;
    button->state = IDLE;    
    button->getButtonVal = getButtonVal;
    button->eventHandler[SINGLE_CLICK] = NULL;
    button->eventHandler[DOUBLE_CLICK] = NULL;
    button->eventHandler[LONG_PRESS]   = NULL;
}

/**
 * @brief ����ע�ᰴ���¼���ͬʱ�������Ӧ�¼�����ʱ�Ļص�����
 * 
 * @param button ����İ������ƿ�
 * @param buttonEvent ��Ҫע����¼����ͣ�������˫����������
 * @param eventHandler ���봥����Ӧ�¼�ʱ�Ļص�����
 * 
 * @note û��ע������¼���������ʹ�������Ӧ�Ķ���Ҳ��������Ӧ
 */
void buttonLink(button_t *button, buttonEvent_t buttonEvent, eventHandler_t eventHandler)
{
    button->eventHandler[buttonEvent] = eventHandler;
}

/**
 * @brief ����״̬��ɨ�躯������Ҫ�Թ̶�10ms����ɨ��
 * 
 * @param button ����İ������ƿ�
 */
void buttonScan(button_t *button)
{
    button->val = button->getButtonVal();
    switch (button->state)
    {
        case IDLE:
        {
            if((button->val != button->idleLevel) && (button->valPre == button->idleLevel))
            {
                button->state = FIRST_PRESS;            
            }
            break;
        }
        case FIRST_PRESS:
        {
            if(button->val == button->valPre)
            {
                button->ticks++;
                if(button->ticks >= button->longPress_Threshold)
                {
                    button->ticks = 0;
                    button->state = IDLE;
                    if(button->eventHandler[LONG_PRESS] != NULL)
                    {
                        button->eventHandler[LONG_PRESS]();
                    }    
                }               
            }
            else
            {
                button->ticks = 0;
                button->state = FIRST_RELEASE;
            }
            break;
        }
        case FIRST_RELEASE:
        {
            if(button->val == button->valPre)
            {
                button->ticks++;
                if(button->ticks >= button->doublePress_Threshold)
                {
                    button->ticks = 0;
                    button->state = IDLE;
                    if(button->eventHandler[SINGLE_CLICK] != NULL)
                    {
                        button->eventHandler[SINGLE_CLICK]();
                    }
                }
            }
            else
            {
                button->ticks = 0;
                button->state = SECOND_PRESS;
            }
            break;
        }
        case SECOND_PRESS:
        {
            if(button->val != button->valPre)
            {
                button->state = IDLE;
                if(button->eventHandler[DOUBLE_CLICK] != NULL)
                {
                    button->eventHandler[DOUBLE_CLICK]();
                }
            }
            break;
        }
        default:
            break;
    }
    button->valPre = button->val;
}

